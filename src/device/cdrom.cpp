#include "cdrom.h"
#include <cassert>
#include <cstdio>
#include "config.h"
#include "sound/audio_cd.h"
#include "system.h"
#include "utils/bcd.h"
#include "utils/string.h"
#include "device/dma3Channel.h"

// TODO: CDROM shouldn't know about DMA channels
#define dma3 dynamic_cast<device::dma::dmaChannel::DMA3Channel*>(sys->dma->dma[3].get())

namespace device {
namespace cdrom {
CDROM::CDROM(System* sys) : sys(sys) { verbose = config["debug"]["log"]["cdrom"]; }

void CDROM::step() {
    status.transmissionBusy = 0;
    if (!CDROM_interrupt.empty()) {
        if ((interruptEnable & 7) & (CDROM_interrupt.front() & 7)) {
            sys->interrupt->trigger(interrupt::CDROM);
        }
    }

    if (stat.read) {
        static int cnt = 0;

        if (++cnt == 500) {
            cnt = 0;
            ackMoreData();
        }
    }

    static int reportcnt = 0;
    if (mode.cddaReport && stat.play && reportcnt++ == 4000) {
        reportcnt = 0;
        // Report--> INT1(stat, track, index, mm / amm, ss + 80h / ass, sect / asect, peaklo, peakhi)
        auto pos = AudioCD::currentPosition;

        int track = 0;
        for (int i = 0; i < cue.getTrackCount(); i++) {
            if (pos >= (cue.tracks[i].start - cue.tracks[i].pause) && pos < cue.tracks[i].end) {
                track = i;
                break;
            }
        }

        auto posInTrack = pos - cue.tracks[track].start;

        CDROM_interrupt.push_back(1);
        writeResponse(stat._reg);           // stat
        writeResponse(track);               // track
        writeResponse(0x01);                // index
        writeResponse(bcd::toBcd(pos.mm));  // minute (disc)
        writeResponse(bcd::toBcd(pos.ss));  // second (disc)
        writeResponse(bcd::toBcd(pos.ff));  // sector (disc)
        writeResponse(bcd::toBcd(0));       // peaklo
        writeResponse(bcd::toBcd(0));       // peakhi

        if (verbose) {
            printf("CDROM: CDDA report -> (");
            for (auto r : CDROM_response) {
                printf("0x%02x,", r);
            }
            printf(")\n");
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
            response = CDROM_response.front();
            CDROM_response.pop_front();

            if (CDROM_response.empty()) {
                status.responseFifoEmpty = 0;
            }
        }
        if (verbose == 2) printf("CDROM: R RESPONSE: 0x%02x\n", response);
        return response;
    }
    if (address == 2) {  // CD Data
        return dma3->readByte();
    }
    if (address == 3) {                                // CD Interrupt enable / flags
        if (status.index == 0 || status.index == 2) {  // Interrupt enable
            if (verbose == 2) printf("CDROM: R INTE: 0x%02x\n", interruptEnable);
            return interruptEnable;
        }
        if (status.index == 1 || status.index == 3) {  // Interrupt flags
            uint8_t _status = 0b11100000;
            if (!CDROM_interrupt.empty()) {
                _status |= CDROM_interrupt.front() & 7;
            }
            if (verbose == 2) printf("CDROM: R INTF: 0x%02x\n", _status);
            return _status;
        }
    }
    printf("CDROM%d.%d->R    ?????\n", address, status.index);
    sys->state = System::State::pause;
    return 0;
}

void CDROM::debugLog(const char* cmd) {
    if (!verbose) return;

    std::string log = "CDROM: ";
    log += cmd;

    if (!CDROM_params.empty()) {
        log += "(";
        for (size_t i = 0; i < CDROM_params.size(); i++) {
            log += string_format("0x%02x", CDROM_params[i]);
            if (i < CDROM_params.size() - 1) log += ", ";
        }
        log += ")";
    }

    if (!CDROM_response.empty()) {
        log += " -> (";
        for (size_t i = 0; i < CDROM_response.size(); i++) {
            log += string_format("0x%02x", CDROM_response[i]);
            if (i < CDROM_response.size() - 1) log += ", ";
        }
        log += ")";
    }

    log += "\n";
}

void CDROM::cmdGetstat() {
    CDROM_interrupt.push_back(3);
    writeResponse(stat._reg);

    if (verbose) printf("CDROM: cmdGetstat -> 0x%02x\n", CDROM_response[0]);
}

void CDROM::cmdSetloc() {
    uint8_t minute = bcd::toBinary(readParam());
    uint8_t second = bcd::toBinary(readParam());
    uint8_t sector = bcd::toBinary(readParam());

    readSector = sector + (second * 75) + (minute * 60 * 75);
    dma3->seekTo(readSector);

    CDROM_interrupt.push_back(3);
    writeResponse(stat._reg);

    if (verbose) printf("CDROM: cmdSetloc(min = %d, sec = %d, sect = %d)\n", minute, second, sector);
}

void CDROM::cmdPlay() {
    // Play NOT IMPLEMENTED
    utils::Position pos;
    if (!CDROM_params.empty()) {
        int track = readParam();  // param or setloc used
        if (track >= cue.getTrackCount()) {
            printf("CDROM: Invalid PLAY track parameter (%d)\n", track);
            return;
        }
        pos = cue.tracks[track].start;
        if (verbose) printf("CDROM: PLAY (track: %d)\n", track);
    } else {
        pos = utils::Position::fromLba(readSector);
    }

    if (verbose) printf("CDROM: PLAY (pos: %s)\n", pos.toString().c_str());
    stat.setMode(StatusCode::Mode::Playing);

    CDROM_interrupt.push_back(3);
    writeResponse(stat._reg);

    AudioCD::play(cue, pos);

    if (verbose) printf("CDROM: cmdPlay (pos: %s)\n", pos.toString().c_str());
}

void CDROM::cmdReadN() {
    stat.setMode(StatusCode::Mode::Reading);

    CDROM_interrupt.push_back(3);
    writeResponse(stat._reg);

    if (verbose) printf("CDROM: cmdReadN\n");
}

void CDROM::cmdMotorOn() {
    stat.motor = 1;

    CDROM_interrupt.push_back(3);
    writeResponse(stat._reg);

    CDROM_interrupt.push_back(2);
    writeResponse(stat._reg);

    if (verbose) printf("CDROM: cmdMotorOn\n");
}

void CDROM::cmdStop() {
    if (verbose) printf("CDROM: STOP\n");
    stat.setMode(StatusCode::Mode::None);
    stat.motor = 0;

    CDROM_interrupt.push_back(3);
    writeResponse(stat._reg);

    CDROM_interrupt.push_back(2);
    writeResponse(stat._reg);

    AudioCD::stop();

    if (verbose) printf("CDROM: cmdStop\n");
}

void CDROM::cmdPause() {
    if (verbose) printf("CDROM: PAUSE\n");
    CDROM_interrupt.push_back(3);
    writeResponse(stat._reg);

    stat.setMode(StatusCode::Mode::None);

    CDROM_interrupt.push_back(2);
    writeResponse(stat._reg);

    AudioCD::stop();

    if (verbose) printf("CDROM: cmdPause\n");
}

void CDROM::cmdInit() {
    CDROM_interrupt.push_back(3);
    writeResponse(stat._reg);

    stat.motor = 1;
    stat.setMode(StatusCode::Mode::None);

    mode._reg = 0;
    dma3->sectorSize = mode.sectorSize;

    CDROM_interrupt.push_back(2);
    writeResponse(stat._reg);

    if (verbose) printf("CDROM: cmdInit\n");
}

void CDROM::cmdMute() {
    CDROM_interrupt.push_back(3);
    writeResponse(stat._reg);

    if (verbose) printf("CDROM: cmdMute\n");
}

void CDROM::cmdDemute() {
    CDROM_interrupt.push_back(3);
    writeResponse(stat._reg);

    if (verbose) printf("CDROM: cmdDemute\n");
}

void CDROM::cmdSetFilter() {
    // TODO: Stub!
    int file = readParam();
    int channel = readParam();
    CDROM_interrupt.push_back(3);
    writeResponse(stat._reg);

    if (verbose) printf("CDROM: cmdSetFilter(file = 0x%02x, ch = 0x%02x)\n", file, channel);
}

void CDROM::cmdSetmode() {
    uint8_t setmode = readParam();

    mode._reg = setmode;

    dma3->sectorSize = mode.sectorSize;

    CDROM_interrupt.push_back(3);
    writeResponse(stat._reg);

    if (verbose) printf("CDROM: cmdSetmode: 0x%02x\n", setmode);
}

void CDROM::cmdSetSession() {
    uint8_t session = readParam();

    CDROM_interrupt.push_back(3);
    writeResponse(stat._reg);

    CDROM_interrupt.push_back(2);
    writeResponse(stat._reg);

    if (verbose) printf("CDROM: cmdSetSession(0x%02x)\n", session);
}

void CDROM::cmdSeekP() {
    if (verbose) printf("CDROM: SEEKP\n");
    CDROM_interrupt.push_back(3);
    writeResponse(stat._reg);

    stat.setMode(StatusCode::Mode::Seeking);

    CDROM_interrupt.push_back(2);
    writeResponse(stat._reg);

    stat.setMode(StatusCode::Mode::None);

    if (verbose) printf("CDROM: cmdSeekP\n");
}

void CDROM::cmdGetlocL() {
    if (verbose) printf("CDROM: cmdGetlocL\n");
    // TODO: Invalid implementation, but allows GTA2 to run
    uint32_t tmp = readSector + 2 * 75;
    uint32_t minute = tmp / 75 / 60;
    uint32_t second = (tmp / 75) % 60;
    uint32_t sector = tmp % 75;

    CDROM_interrupt.push_back(3);
    writeResponse(bcd::toBcd(minute));  // minute (track)
    writeResponse(bcd::toBcd(second));  // second (track)
    writeResponse(bcd::toBcd(sector));  // sector (track)
    writeResponse(bcd::toBcd(minute));  // mode
    writeResponse(bcd::toBcd(0x00));    // file
    writeResponse(bcd::toBcd(0x00));    // channel
    writeResponse(bcd::toBcd(0x08));    // sm
    writeResponse(bcd::toBcd(0x00));    // ci

    if (verbose) {
        printf("CDROM: cmdGetlocL -> (");
        for (auto r : CDROM_response) {
            printf("0x%02x,", r);
        }
        printf(")\n");
    }
}

void CDROM::cmdGetlocP() {
    auto pos = AudioCD::currentPosition;

    int track = 0;
    for (int i = 0; i < cue.getTrackCount(); i++) {
        if (pos >= (cue.tracks[i].start - cue.tracks[i].pause) && pos < cue.tracks[i].end) {
            track = i;
            break;
        }
    }

    auto posInTrack = pos - cue.tracks[track].start;

    CDROM_interrupt.push_back(3);
    writeResponse(track);                      // track
    writeResponse(0x01);                       // index
    writeResponse(bcd::toBcd(posInTrack.mm));  // minute (track)
    writeResponse(bcd::toBcd(posInTrack.ss));  // second (track)
    writeResponse(bcd::toBcd(posInTrack.ff));  // sector (track)
    writeResponse(bcd::toBcd(pos.mm));         // minute (disc)
    writeResponse(bcd::toBcd(pos.ss));         // second (disc)
    writeResponse(bcd::toBcd(pos.ff));         // sector (disc)

    if (verbose) {
        printf("CDROM: cmdGetlocP -> (");
        for (auto r : CDROM_response) {
            printf("0x%02x,", r);
        }
        printf(")\n");
    }
}

void CDROM::cmdGetTN() {
    CDROM_interrupt.push_back(3);
    writeResponse(stat._reg);
    writeResponse(0x01);
    writeResponse(cue.getTrackCount());

    if (verbose) {
        printf("CDROM: cmdGetTN -> (");
        for (auto r : CDROM_response) {
            printf("0x%02x,", r);
        }
        printf(")\n");
    }
}

void CDROM::cmdGetTD() {
    int track = readParam();
    CDROM_interrupt.push_back(3);
    writeResponse(stat._reg);
    if (track == 0)  // end of last track
    {
        auto diskSize = cue.getDiskSize();
        if (verbose) printf("GetTD(0): minute: %d, second: %d\n", diskSize.mm, diskSize.ss);

        writeResponse(bcd::toBcd(diskSize.mm));
        writeResponse(bcd::toBcd(diskSize.ss));
    } else {  // Start of n track
        if (track > cue.getTrackCount()) {
            // Error
            return;
        }

        auto start = cue.tracks[track - 1].start - cue.tracks[track - 1].pause;

        if (verbose) printf("GetTD(%d): minute: %d, second: %d\n", track, start.mm, start.ss);
        writeResponse(bcd::toBcd(start.mm));
        writeResponse(bcd::toBcd(start.ss));
    }

    if (verbose) {
        printf("CDROM: cmdGetTD(0x%02x) -> (", track);
        for (auto r : CDROM_response) {
            printf("0x%02x,", r);
        }
        printf(")\n");
    }
}

void CDROM::cmdSeekL() {
    dma3->seekTo(readSector);

    CDROM_interrupt.push_back(3);
    writeResponse(stat._reg);

    stat.setMode(StatusCode::Mode::Seeking);

    CDROM_interrupt.push_back(2);
    writeResponse(stat._reg);

    status.dataFifoEmpty = 0;

    if (verbose) printf("CDROM: cmdSeekL\n");
}

void CDROM::cmdTest() {
    uint8_t opcode = readParam();
    if (opcode == 0x20)  // Get CDROM BIOS date/version (yy,mm,dd,ver)
    {
        CDROM_interrupt.push_back(3);
        writeResponse(0x94);
        writeResponse(0x09);
        writeResponse(0x19);
        writeResponse(0xc0);
    } else if (opcode == 0x03) {  // Force motor off, used in swap
        stat.motor = 0;
        CDROM_interrupt.push_back(3);
        writeResponse(stat._reg);
    } else {
        printf("Unimplemented test CDROM opcode (0x%x)!\n", opcode);
        CDROM_interrupt.push_back(5);
    }

    if (verbose) {
        printf("CDROM: cmdTest(0x%02x) -> (", opcode);
        for (auto r : CDROM_response) {
            printf("0x%02x,", r);
        }
        printf(")\n");
    }
}

void CDROM::cmdGetId() {
    // Shell open
    if (stat.getShell()) {
        CDROM_interrupt.push_back(5);
        writeResponse(0x11);
        writeResponse(0x80);
        return;
    }

    // Comment to make NO$PSX bios works
    CDROM_interrupt.push_back(3);
    writeResponse(stat._reg);

    // No CD
    if (cue.getTrackCount() == 0) {
        CDROM_interrupt.push_back(5);
        writeResponse(0x08);
        writeResponse(0x40);
        for (int i = 0; i < 6; i++) writeResponse(0);
    }
    // Audio CD
    else if (cue.tracks[0].type == utils::Track::Type::AUDIO) {
        CDROM_interrupt.push_back(5);
        writeResponse(0x0a);
        writeResponse(0x90);
        for (int i = 0; i < 6; i++) writeResponse(0);
    } else {
        // Game C
        CDROM_interrupt.push_back(2);
        writeResponse(0x02);
        writeResponse(0x00);
        writeResponse(0x20);
        writeResponse(0x00);
        writeResponse('S');
        writeResponse('C');
        writeResponse('E');
        writeResponse('A');  // 0x45 E, 0x41 A, 0x49 I
    }

    if (verbose) {
        printf("CDROM: cmdGetId -> (");
        for (auto r : CDROM_response) {
            printf("0x%02x,", r);
        }
        printf(")\n");
    }
}

void CDROM::cmdReadS() {
    stat.setMode(StatusCode::Mode::Reading);

    CDROM_interrupt.push_back(3);
    writeResponse(stat._reg);

    if (verbose) printf("CDROM: cmdReadS\n");
}

void CDROM::cmdReadTOC() {
    CDROM_interrupt.push_back(3);
    writeResponse(stat._reg);

    CDROM_interrupt.push_back(2);
    writeResponse(stat._reg);

    if (verbose) printf("CDROM: cmdReadTOC\n");
}

void CDROM::cmdUnlock() {
    // Semi implemented
    CDROM_interrupt.push_back(5);
    writeResponse(0x11);
    writeResponse(0x40);

    if (verbose) {
        printf("CDROM: cmdUnlock -> (");
        for (auto r : CDROM_response) {
            printf("0x%02x,", r);
        }
        printf(")\n");
    }
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
        CDROM_interrupt.push_back(5);
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
        CDROM_params.push_back(data);
        status.parameterFifoEmpty = 0;
        status.parameterFifoFull = !(CDROM_params.size() >= 16);
        return;
    }
    if (address == 3 && status.index == 0) {  // Request register
        static int state = 0;
        if (data & 0x80) {  // want data
            if (state == 1) {
                state = 0;
                // advance sector
                dma3->advanceSector();
            }
        } else {  // clear data fifo
            // status.dataFifoEmpty = 0;
            if (state == 0) state = 1;
        }

        // 0x00, 0x80,  get next sector?
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
            CDROM_interrupt.pop_front();
        }
        if (verbose == 2) printf("CDROM: W INTF: 0x%02x\n", data);
        return;
    }

    if (address == 2 && status.index == 2) {  // Left CD to Left SPU
        return;
    }

    if (address == 3 && status.index == 2) {  // Left CD to Right SPU
        return;
    }

    if (address == 1 && status.index == 3) {  // Right CD to Right SPU
        return;
    }

    if (address == 2 && status.index == 3) {  // Right CD to Left SPU
        return;
    }

    if (address == 3 && status.index == 3) {  // Apply volume changes (bit5)
        return;
    }

    printf("CDROM%d.%d<-W  UNIMPLEMENTED WRITE       0x%02x\n", address, status.index, data);
    sys->state = System::State::pause;
}
}  // namespace cdrom
}  // namespace device
