#include "cdrom.h"
#include <fmt/core.h>
#include <cassert>
#include <disc/track.h>
#include "config.h"
#include "disc/empty.h"
#include "sound/adpcm.h"
#include "system.h"
#include "utils/bcd.h"
#include "utils/cd.h"

namespace device {
namespace cdrom {

CDROM::CDROM(System* sys) : sys(sys) {
    verbose = config.debug.log.cdrom;
    disc = std::make_unique<disc::Empty>();
}

void CDROM::handleSector() {
    if (!stat.read && !stat.play) return;

    const std::array<uint8_t, 12> sync = {{0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00}};

    auto pos = disc::Position::fromLba(readSector);
    std::tie(rawSector, trackType) = disc->read(pos);
    auto q = disc->getSubQ(pos);
    if (q.validCrc()) {
        this->lastQ = q;
    }
    readSector++;

    if (trackType == disc::TrackType::AUDIO && stat.play) {
        if (!mode.cddaEnable) {
            return;
        }

        if (memcmp(rawSector.data(), sync.data(), sync.size()) == 0) {
            fmt::print("[CDROM] Trying to read Data track as audio\n");
            return;
        }

        if (mode.cddaReport) {
            // Report--> INT1(stat, track, index, mm / amm, ss + 80h / ass, sect / asect, peaklo, peakhi)
            auto pos = disc::Position::fromLba(readSector);
            int track = disc->getTrackByPosition(pos);

            auto posInTrack = pos - disc->getTrackStart(track);

            postInterrupt(1);
            writeResponse(stat._reg);           // stat
            writeResponse(bcd::toBcd(track));   // track
            writeResponse(0x01);                // index
            writeResponse(bcd::toBcd(posInTrack.mm));         // minute (disc) <<< invalid
            writeResponse(bcd::toBcd(posInTrack.ss) | 0x80);  // second (disc) <<< invalid
            writeResponse(bcd::toBcd(posInTrack.ff));         // sector (disc)
            writeResponse(bcd::toBcd(0));       // peaklo
            writeResponse(bcd::toBcd(0));       // peakhi

            if (verbose) {
                fmt::print("CDROM:CDDA report -> ({})\n", dumpFifo(interruptQueue.peek().response));
            }
        }

        if (!mute) {
            // Decode Red Book Audio (16bit Stereo 44100Hz)
            for (size_t i = 0; i < rawSector.size(); i += 4) {
                int16_t left = rawSector[i + 0] | (rawSector[i + 1] << 8);
                int16_t right = rawSector[i + 2] | (rawSector[i + 3] << 8);

                audio.push_back(mixSample(std::make_pair(left, right)));
            }
        }
    } else if (trackType == disc::TrackType::DATA && stat.read) {
        ackMoreData();

        if (memcmp(rawSector.data(), sync.data(), sync.size()) != 0) {
            fmt::print("CDROM: Invalid sync\n");
            return;
        }

        // uint8_t minute = rawSector[12];
        // uint8_t second = rawSector[13];
        // uint8_t frame = rawSector[14];
        uint8_t mode = rawSector[15];

        uint8_t file = rawSector[16];
        uint8_t channel = rawSector[17];
        auto submode = static_cast<cd::Submode>(rawSector[18]);
        auto codinginfo = static_cast<cd::Codinginfo>(rawSector[19]);

        // XA uses Mode2 sectors
        // Does PSX even support Mode1?
        if (mode != 2) {
            fmt::print("CDROM: Not mode2 ({} instead)\n", mode);
            return;
        }

        // Only Form2 ?
        // Does PSX support Form1?
        // Streaming
        if (submode.form2 && submode.realtime) {
            // Filter XA file/channel
            if (this->mode.xaFilter && (filter.file != file || filter.channel != channel)) {
                return;
            }

            // Only realtime audio
            if (!submode.audio || !submode.realtime) {
                return;
            }

            if (codinginfo.bits == 1) {
                fmt::print("[CDROM] Unsupported 8bit mode for XA\n");
                exit(1);
            }

            if (this->mode.xaEnabled && !this->mute) {
                auto frame = ADPCM::decodeXA(rawSector.data() + 24, codinginfo);

                for (auto sample : frame) {
                    audio.push_back(mixSample(sample));
                }
            }

            if (submode.endOfFile) {
                fmt::print("CDROM: End of file\n");
                stat.read = false;
                return;
            }
        } else {
            // Plain data
        }
    }
}

void CDROM::step(int cycles) {
    static int intCycles = 0;

    readcnt += cycles;
    intCycles += cycles;

    if (intCycles >= 300) {
        intCycles -= 300;
        status.transmissionBusy = 0;
        if (!interruptQueue.is_empty()) {
            if ((interruptEnable & 7) & (interruptQueue.peek().irq & 7)) {
                sys->interrupt->trigger(interrupt::CDROM);
            }
        }
    }

    const int sectorsPerSecond = mode.speed ? 150 : 75;
    const int cyclesPerSector = timing::CPU_CLOCK / sectorsPerSecond;
    for (int i = 0; i < readcnt / cyclesPerSector; i++) {
        handleSector();
    }
    readcnt %= cyclesPerSector;
}

void CDROM::writeResponse(uint8_t byte) {
    if (interruptQueue.is_empty()) {
        fmt::print("CDROM: Fatal: Trying to write response to empty interruptQueue\n");
        return;
    }
    auto& entry = interruptQueue.ref(interruptQueue.size() - 1);
    if (entry.response.is_full()) {
        return;
    }
    entry.response.add(byte);
}

uint8_t CDROM::read(uint32_t address) {
    if (address == 0) {  // CD Status
        // status.transmissionBusy = !interruptQueue.empty();
        if (verbose == 2) fmt::print("CDROM: R STATUS: 0x{:02x}\n", status._reg);
        bool responseFifoEmpty = interruptQueue.is_empty() || interruptQueue.peek().response.is_empty();

        status.parameterFifoEmpty = CDROM_params.is_empty();
        status.parameterFifoFull = !CDROM_params.is_full();  // Inverse logic
        status.responseFifoEmpty = !responseFifoEmpty;       // Inverse logic
        return status._reg;
    }
    if (address == 1) {  // CD Response
        uint8_t ret = 0;

        if (!interruptQueue.is_empty()) {
            auto& response = interruptQueue.ref();
            if (!response.response.is_empty()) {
                ret = response.response.get();

                if (response.response.is_empty() && response.ack == true) {
                    interruptQueue.get();
                }
            }
        }
        if (verbose == 2) fmt::print("CDROM: R RESPONSE: 0x{:02x}\n", ret);
        return ret;
    }
    if (address == 2) {  // CD Data
        uint8_t byte = readByte();
        if (verbose == 3) fmt::print("CDROM: R DATA: 0x{:02x}\n", byte);
        return byte;
    }
    if (address == 3) {                                // CD Interrupt enable / flags
        if (status.index == 0 || status.index == 2) {  // Interrupt enable
            if (verbose == 2) fmt::print("CDROM: R INTE: 0x{:02x}\n", interruptEnable);
            return interruptEnable;
        }
        if (status.index == 1 || status.index == 3) {  // Interrupt flags
            uint8_t _status = 0b11100000;
            if (!interruptQueue.is_empty()) {
                _status |= interruptQueue.peek().irq & 7;
            }
            if (verbose == 2) fmt::print("CDROM: R INTF: 0x{:02x}\n", _status);
            return _status;
        }
    }
    fmt::print("CDROM{}.{}->R    ?????\n", address, static_cast<int>(status.index));
    return 0;
}

bool CDROM::isBufferEmpty() {
    if (dataBuffer.empty()) return true;

    if (!mode.sectorSize && dataBufferPointer >= 0x800) return true;
    if (mode.sectorSize && dataBufferPointer >= 0x924) return true;

    return false;
}

uint8_t CDROM::readByte() {
    if (dataBuffer.empty()) {
        fmt::print("[CDROM] Buffer empty\n");
        return 0;
    }

    // 0 - 0x800 - just data
    // 1 - 0x924 - whole sector without sync
    int dataStart = 12;
    if (!mode.sectorSize) dataStart += 12;

    // docs says that reading outside of data buffer returns these value
    if (!mode.sectorSize && dataBufferPointer >= 0x800) {
        dataBufferPointer++;
        return dataBuffer[dataStart + 0x800 - 8];
    } else if (mode.sectorSize && dataBufferPointer >= 0x924) {
        dataBufferPointer++;
        return dataBuffer[dataStart + 0x924 - 4];
    }

    uint8_t data = dataBuffer[dataStart + dataBufferPointer++];

    if (isBufferEmpty()) {
        status.dataFifoEmpty = 0;
    }

    return data;
}

std::string CDROM::dumpFifo(const FIFO& f) {
    std::string log = "";
    for (size_t i = 0u; i < f.size(); i++) {
        log += fmt::format("0x{:02x}", f[i]);
        if (i < f.size() - 1) log += ", ";
    }
    return log;
}

void CDROM::handleCommand(uint8_t cmd) {
    interruptQueue.clear();
    switch (cmd) {
        case 0x01: cmdGetstat(); break;
        case 0x02: cmdSetloc(); break;
        case 0x03: cmdPlay(); break;
        // Missing 0x04 cmdForward
        // Missing 0x05 cmdBackward
        case 0x06: cmdReadN(); break;
        case 0x07: cmdMotorOn(); break;
        case 0x08: cmdStop(); break;
        case 0x09: cmdPause(); break;
        case 0x0a: cmdInit(); break;
        case 0x0b: cmdMute(); break;
        case 0x0c: cmdDemute(); break;
        case 0x0d: cmdSetFilter(); break;
        case 0x0e: cmdSetmode(); break;
        case 0x0f: cmdGetparam(); break;
        case 0x10: cmdGetlocL(); break;
        case 0x11: cmdGetlocP(); break;
        case 0x12: cmdSetSession(); break;
        case 0x13: cmdGetTN(); break;
        case 0x14: cmdGetTD(); break;
        case 0x15: cmdSeekL(); break;
        case 0x16: cmdSeekP(); break;
        // Missing 0x17 ???
        // Missing 0x18 ???
        case 0x19: cmdTest(); break;
        case 0x1a: cmdGetId(); break;
        case 0x1b: cmdReadS(); break;
        // Missing 0x1c cmdReset
        // Missing 0x1d cmdGetQ
        case 0x1e: cmdReadTOC(); break;
        case 0x50:
        case 0x51:
        case 0x52:
        case 0x53:
        case 0x54:
        case 0x55:
        case 0x56: cmdUnlock(); break;
        default:
            fmt::print("[CDROM] Unimplemented cmd 0x{:x}, please report this game/software on project page.\n", cmd);
            postInterrupt(5);
            writeResponse(0x11);
            writeResponse(0x40);
            break;
    }

    CDROM_params.clear();
    status.transmissionBusy = 1;
    status.xaFifoEmpty = 0;
}

void CDROM::write(uint32_t address, uint8_t data) {
    if (address == 0) {
        if (verbose == 3) fmt::print("CDROM: W INDEX: 0x{:02x}\n", data);
        status.index = data & 3;
        return;
    }
    if (address == 1 && status.index == 0) {  // Command register
        if (verbose == 3) fmt::print("CDROM: W COMMAND: 0x{:02x}\n", data);
        return handleCommand(data);
    }
    if (address == 2 && status.index == 0) {  // Parameter fifo
        CDROM_params.add(data);
        if (verbose == 3) fmt::print("CDROM: W PARAMFIFO: 0x{:02x}\n", data);
        return;
    }
    if (address == 3 && status.index == 0) {  // Request register
        if (data & 0x80) {                    // want data
            // Load fifo only when buffer is empty
            if (isBufferEmpty()) {
                dataBuffer = rawSector;
                dataBufferPointer = 0;
                status.dataFifoEmpty = 1;
            }
        } else {  // clear data fifo
            status.dataFifoEmpty = 0;
            dataBuffer.clear();
            dataBufferPointer = 0;
        }

        if (verbose == 2) fmt::print("CDROM: W REQDATA: 0x{:02x}\n", data);
        return;
    }
    if (address == 2 && status.index == 1) {  // Interrupt enable
        interruptEnable = data;
        if (verbose == 2) fmt::print("CDROM: W INTE: 0x{:02x}\n", data);
        return;
    }
    if (address == 3 && status.index == 1) {  // Interrupt flags
        if (data & 0x40)                      // reset parameter fifo
        {
            CDROM_params.clear();
        }

        if (!interruptQueue.is_empty()) {
            interruptQueue.ref().ack = true;
            if (interruptQueue.ref().response.is_empty()) {
                interruptQueue.get();
            }
        }
        if (verbose == 2) fmt::print("CDROM: W INTF: 0x{:02x}\n", data);
        return;
    }

    if (address == 2 && status.index == 2) {  // Left CD to Left SPU
        volumeLeftToLeft = data;
        if (verbose == 2) fmt::print("CDROM: W LeftCDtoLeftSPU: 0x{:02x}\n", data);
        return;
    }

    if (address == 3 && status.index == 2) {  // Left CD to Right SPU
        volumeLeftToRight = data;
        if (verbose == 2) fmt::print("CDROM: W LeftCDtoRightSPU: 0x{:02x}\n", data);
        return;
    }

    if (address == 1 && status.index == 3) {  // Right CD to Right SPU
        volumeRightToRight = data;
        if (verbose == 2) fmt::print("CDROM: W RightCDtoRightSPU: 0x{:02x}\n", data);
        return;
    }

    if (address == 2 && status.index == 3) {  // Right CD to Left SPU
        volumeRightToLeft = data;
        if (verbose == 2) fmt::print("CDROM: W RightCDtoLeftSPU: 0x{:02x}\n", data);
        return;
    }

    if (address == 3 && status.index == 3) {  // Apply volume changes (bit5)
        // FIXME: Temporarily ignored
        if (verbose == 2) fmt::print("CDROM: W ApplyVolumeChanges: 0x{:02x}\n", data);
        return;
    }

    fmt::print("CDROM{}.{}<-W  UNIMPLEMENTED WRITE       0x{:02x}\n", address, static_cast<int>(status.index), data);
}

std::pair<int16_t, int16_t> CDROM::mixSample(std::pair<int16_t, int16_t> input) {
    int16_t left = input.first;
    int16_t right = input.second;

    // TODO: Fixed-point + clipping
    // TODO: Verify mixing with HW (capture channels)
    // 0x00 - disabled
    // 0x80 - 1x vol
    // 0xff - 2x vol
    float l_l = volumeLeftToLeft / (float)0x80;
    float l_r = volumeLeftToRight / (float)0x80;
    float r_l = volumeRightToLeft / (float)0x80;
    float r_r = volumeRightToRight / (float)0x80;

    int16_t mixedLeft = clamp<int32_t>((left * l_l) + (right * r_l), INT16_MIN, INT16_MAX);
    int16_t mixedRight = clamp<int32_t>((left * l_r) + (right * r_r), INT16_MIN, INT16_MAX);

    return std::make_pair(mixedLeft, mixedRight);
}

}  // namespace cdrom
}  // namespace device
