#include "cdrom.h"
#include <cassert>
#include <cstdio>
#include "config.h"
#include "disc/empty.h"
#include "sound/adpcm.h"
#include "system.h"
#include "utils/bcd.h"
#include "utils/cd.h"
#include "utils/string.h"

namespace device {
namespace cdrom {

CDROM::CDROM(System* sys) : sys(sys) {
    verbose = config["debug"]["log"]["cdrom"];
    disc = std::make_unique<disc::Empty>();
}

void CDROM::step() {
    status.transmissionBusy = 0;
    if (!CDROM_interrupt.empty()) {
        if ((interruptEnable & 7) & (CDROM_interrupt.peek() & 7)) {
            sys->interrupt->trigger(interrupt::CDROM);
        }
    }

    static int readcnt = 0;
    if ((stat.read || stat.play) && readcnt++ == 1150) {  // FIXME: yey, magic numbers
        readcnt = 0;
        const std::array<uint8_t, 12> sync = {{0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00}};

        auto pos = disc::Position::fromLba(readSector);
        std::tie(rawSector, trackType) = disc->read(pos);
        readSector++;

        if (trackType == disc::TrackType::AUDIO && stat.play) {
            if (!mode.cddaEnable) {
                return;
            }

            if (memcmp(rawSector.data(), sync.data(), sync.size()) == 0) {
                printf("[CDROM] Trying to read Data track as audio\n");
                return;
            }

            if (mode.cddaReport) {
                // Report--> INT1(stat, track, index, mm / amm, ss + 80h / ass, sect / asect, peaklo, peakhi)
                auto pos = disc::Position::fromLba(readSector);

                int track = disc->getTrackByPosition(pos);

                CDROM_interrupt.add(1);
                writeResponse(stat._reg);           // stat
                writeResponse(bcd::toBcd(track));   // track
                writeResponse(0x01);                // index
                writeResponse(bcd::toBcd(pos.mm));  // minute (disc)
                writeResponse(bcd::toBcd(pos.ss));  // second (disc)
                writeResponse(bcd::toBcd(pos.ff));  // sector (disc)
                writeResponse(bcd::toBcd(0));       // peaklo
                writeResponse(bcd::toBcd(0));       // peakhi

                if (verbose) {
                    printf("CDROM: CDDA report -> (%s)\n", dumpFifo(CDROM_response).c_str());
                }
            }

            if (!this->mute) {
                // Decode Red Book Audio (16bit Stereo 44100Hz)
                bool channel = true;
                for (size_t i = 0; i < rawSector.size(); i += 2) {
                    int16_t sample = rawSector[i] | (rawSector[i + 1] << 8);

                    if (channel) {
                        audio.first.push_back(sample);
                    } else {
                        audio.second.push_back(sample);
                    }
                    channel = !channel;
                }
            }
        } else if (trackType == disc::TrackType::DATA && stat.read) {
            ackMoreData();

            if (memcmp(rawSector.data(), sync.data(), sync.size()) != 0) {
                printf("Invalid sync\n");
                return;
            }

            uint8_t minute = rawSector[12];
            uint8_t second = rawSector[13];
            uint8_t frame = rawSector[14];
            uint8_t mode = rawSector[15];

            uint8_t file = rawSector[16];
            uint8_t channel = rawSector[17];
            auto submode = static_cast<cd::Submode>(rawSector[18]);
            auto codinginfo = static_cast<cd::Codinginfo>(rawSector[19]);

            // XA uses Mode2 sectors
            // Does PSX even support Mode1?
            if (mode != 2) {
                printf("Not mode2 (%d instead)\n", mode);
                return;
            }

            // Only Form2 ?
            // Does PSX support Form1?
            // Streaming
            if (submode.form2 && submode.realtime) {
                // Filter XA file/channel
                if (this->mode.xaFilter && (filter.file != file || filter.channel != channel)) {
                    // printf("Mismatch filter\n");
                    return;
                }

                // Only realtime audio
                if (!submode.audio || !submode.realtime) {
                    // printf("!audio || !realtime\n");
                    return;
                }

                if (codinginfo.bits == 1) {
                    printf("[CDROM] Unsupported 8bit mode for XA\n");
                    exit(1);
                }

                if (this->mode.xaEnabled && !this->mute) {
                    auto [left, right] = ADPCM::decodeXA(rawSector.data() + 24, codinginfo);
                    audio.first.insert(audio.first.end(), left.begin(), left.end());
                    audio.second.insert(audio.second.end(), right.begin(), right.end());
                }

                if (submode.endOfFile) {
                    printf("End of file\n");
                    stat.read = false;
                    return;
                }
            } else {
                // Plain data
            }
        }
    }
}

uint8_t CDROM::read(uint32_t address) {
    if (address == 0) {  // CD Status
        // status.transmissionBusy = !CDROM_interrupt.empty();
        if (verbose == 2) printf("CDROM: R STATUS: 0x%02x\n", status._reg);
        return status._reg;
    }
    if (address == 1) {  // CD Response
        uint8_t response = 0;
        if (!CDROM_response.empty()) {
            response = CDROM_response.get();

            if (CDROM_response.empty()) {
                status.responseFifoEmpty = 0;
            }
        }
        if (verbose == 2) printf("CDROM: R RESPONSE: 0x%02x\n", response);
        return response;
    }
    if (address == 2) {  // CD Data
        return readByte();
    }
    if (address == 3) {                                // CD Interrupt enable / flags
        if (status.index == 0 || status.index == 2) {  // Interrupt enable
            if (verbose == 2) printf("CDROM: R INTE: 0x%02x\n", interruptEnable);
            return interruptEnable;
        }
        if (status.index == 1 || status.index == 3) {  // Interrupt flags
            uint8_t _status = 0b11100000;
            if (!CDROM_interrupt.empty()) {
                _status |= CDROM_interrupt.peek() & 7;
            }
            if (verbose == 2) printf("CDROM: R INTF: 0x%02x\n", _status);
            return _status;
        }
    }
    printf("CDROM%d.%d->R    ?????\n", address, status.index);
    sys->state = System::State::pause;
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
        printf("[CDROM] Buffer empty\n");
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

std::string CDROM::dumpFifo(const fifo<16, uint8_t> f) {
    std::string log = "";
    for (size_t i = 0; i < f.size(); i++) {
        log += string_format("0x%02x", f[i]);
        if (i < f.size() - 1) log += ", ";
    }
    return log;
}

void CDROM::handleCommand(uint8_t cmd) {
    CDROM_interrupt.clear();
    CDROM_response.clear();
    if (cmd == 0x01)
        cmdGetstat();
    else if (cmd == 0x02)
        cmdSetloc();
    else if (cmd == 0x03)
        cmdPlay();
    else if (cmd == 0x06)
        cmdReadN();
    else if (cmd == 0x07)
        cmdMotorOn();
    else if (cmd == 0x1b)
        cmdReadS();
    else if (cmd == 0x0e)
        cmdSetmode();
    else if (cmd == 0x08)
        cmdStop();
    else if (cmd == 0x09)
        cmdPause();
    else if (cmd == 0x0b)
        cmdMute();
    else if (cmd == 0x0c)
        cmdDemute();
    else if (cmd == 0x0d)
        cmdSetFilter();
    else if (cmd == 0x10)
        cmdGetlocL();
    else if (cmd == 0x11)
        cmdGetlocP();
    else if (cmd == 0x12)
        cmdSetSession();
    else if (cmd == 0x13)
        cmdGetTN();
    else if (cmd == 0x14)
        cmdGetTD();
    else if (cmd == 0x15)
        cmdSeekL();
    else if (cmd == 0x16)
        cmdSeekP();
    else if (cmd == 0x0a)
        cmdInit();
    else if (cmd == 0x19)
        cmdTest();
    else if (cmd == 0x1A)
        cmdGetId();
    else if (cmd == 0x1e)
        cmdReadTOC();
    else if (cmd >= 0x50 && cmd <= 0x56)
        cmdUnlock();
    else {
        printf("Unimplemented cmd 0x%x!\n", cmd);
        CDROM_interrupt.add(5);
        writeResponse(0x11);
        writeResponse(0x40);
    }
    CDROM_params.clear();
    status.parameterFifoEmpty = 1;
    status.parameterFifoFull = 1;
    status.transmissionBusy = 1;
    status.xaFifoEmpty = 0;
}

void CDROM::write(uint32_t address, uint8_t data) {
    if (address == 0) {
        status.index = data & 3;
        return;
    }
    if (address == 1 && status.index == 0) {  // Command register
        return handleCommand(data);
    }
    if (address == 2 && status.index == 0) {  // Parameter fifo
        assert(CDROM_params.size() < 16);
        CDROM_params.add(data);
        status.parameterFifoEmpty = 0;
        status.parameterFifoFull = !(CDROM_params.size() >= 16);
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

        if (verbose == 2) printf("CDROM: W REQDATA: 0x%02x\n", data);
        return;
    }
    if (address == 2 && status.index == 1) {  // Interrupt enable
        interruptEnable = data;
        if (verbose == 2) printf("CDROM: W INTE: 0x%02x\n", data);
        return;
    }
    if (address == 3 && status.index == 1) {  // Interrupt flags
        if (data & 0x40)                      // reset parameter fifo
        {
            CDROM_params.clear();
            status.parameterFifoEmpty = 1;
            status.parameterFifoFull = 1;
        }

        if (!CDROM_interrupt.empty()) {
            CDROM_interrupt.get();
        }
        if (verbose == 2) printf("CDROM: W INTF: 0x%02x\n", data);
        return;
    }

    if (address == 2 && status.index == 2) {  // Left CD to Left SPU
        volumeLeftToLeft = data;
        return;
    }

    if (address == 3 && status.index == 2) {  // Left CD to Right SPU
        volumeLeftToRight = data;
        return;
    }

    if (address == 1 && status.index == 3) {  // Right CD to Right SPU
        volumeRightToRight = data;
        return;
    }

    if (address == 2 && status.index == 3) {  // Right CD to Left SPU
        volumeRightToLeft = data;
        return;
    }

    if (address == 3 && status.index == 3) {  // Apply volume changes (bit5)
        // FIXME: Temporarily ignored
        return;
    }

    printf("CDROM%d.%d<-W  UNIMPLEMENTED WRITE       0x%02x\n", address, status.index, data);
    sys->state = System::State::pause;
}
}  // namespace cdrom
}  // namespace device
