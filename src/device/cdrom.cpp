#include "cdrom.h"
#include "mips.h"
#include <cstdio>
#include <cassert>
#include "utils/bcd.h"

namespace device {
namespace cdrom {
CDROM::CDROM() {}

void CDROM::step() {
    status.transmissionBusy = 0;
    if (!CDROM_interrupt.empty()) {
        if ((interruptEnable & 7) & (CDROM_interrupt.front() & 7)) {
            ((mips::CPU*)_cpu)->interrupt->trigger(interrupt::CDROM);
        }
    }

    if (stat.read) {
        static int cnt = 0;

        if (++cnt == 500) {
            cnt = 0;
            ackMoreData();
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
        return ((mips::CPU*)_cpu)->dma->dma3.readByte();
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
    ((mips::CPU*)_cpu)->state = mips::CPU::State::pause;
    return 0;
}

void CDROM::cmdGetstat() {
    CDROM_interrupt.push_back(3);
    writeResponse(stat._reg);
}

void CDROM::cmdSetloc() {
    uint8_t minute = bcd::toBinary(readParam());
    uint8_t second = bcd::toBinary(readParam());
    uint8_t sector = bcd::toBinary(readParam());
    if (verbose) printf("Setloc: min: %d  sec: %d  sect: %d\n", minute, second, sector);

    readSector = sector + (second * 75) + (minute * 60 * 75);
    readSector -= 2 * 75;
    assert(readSector >= 0);
    ((mips::CPU*)_cpu)->dma->dma3.seekTo(readSector);

    CDROM_interrupt.push_back(3);
    writeResponse(stat._reg);
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
        printf("CDROM: PLAY (track: %d)\n", track);
    } else {
        pos = utils::Position::fromLba(readSector + 150);
    }

    printf("CDROM: PLAY (pos: %s)\n", pos.toString().c_str());
    stat.setMode(StatusCode::Mode::Playing);

    CDROM_interrupt.push_back(3);
    writeResponse(stat._reg);
}

void CDROM::cmdReadN() {
    stat.setMode(StatusCode::Mode::Reading);

    CDROM_interrupt.push_back(3);
    writeResponse(stat._reg);
}

void CDROM::cmdMotorOn() {
    stat.motor = 1;

    CDROM_interrupt.push_back(3);
    writeResponse(stat._reg);

    CDROM_interrupt.push_back(2);
    writeResponse(stat._reg);
}

void CDROM::cmdStop() {
    printf("CDROM: STOP\n");
    stat.setMode(StatusCode::Mode::None);
    stat.motor = 0;

    CDROM_interrupt.push_back(3);
    writeResponse(stat._reg);

    CDROM_interrupt.push_back(2);
    writeResponse(stat._reg);
}

void CDROM::cmdPause() {
    printf("CDROM: PAUSE\n");
    CDROM_interrupt.push_back(3);
    writeResponse(stat._reg);

    stat.setMode(StatusCode::Mode::None);

    CDROM_interrupt.push_back(2);
    writeResponse(stat._reg);
}

void CDROM::cmdInit() {
    CDROM_interrupt.push_back(3);
    writeResponse(stat._reg);

    stat.motor = 1;
    stat.setMode(StatusCode::Mode::None);

    sectorSize = false;
    ((mips::CPU*)_cpu)->dma->dma3.sectorSize = sectorSize;

    CDROM_interrupt.push_back(2);
    writeResponse(stat._reg);
}

void CDROM::cmdMute() {
    CDROM_interrupt.push_back(3);
    writeResponse(stat._reg);
}

void CDROM::cmdDemute() {
    CDROM_interrupt.push_back(3);
    writeResponse(stat._reg);
}

void CDROM::cmdSetFilter() {
    // TODO: Stub!
    int file = readParam();
    int channel = readParam();
    CDROM_interrupt.push_back(3);
    writeResponse(stat._reg);
}

void CDROM::cmdSetmode() {
    uint8_t setmode = CDROM_params.front();
    CDROM_params.pop_front();

    sectorSize = setmode & (1 << 5) ? true : false;
    ((mips::CPU*)_cpu)->dma->dma3.sectorSize = sectorSize;

    CDROM_interrupt.push_back(3);
    writeResponse(stat._reg);
}

void CDROM::cmdSetSession() {
    CDROM_interrupt.push_back(3);
    writeResponse(stat._reg);

    CDROM_interrupt.push_back(2);
    writeResponse(stat._reg);
}

void CDROM::cmdSeekP() {
    printf("CDROM: SEEKP\n");
    CDROM_interrupt.push_back(3);
    writeResponse(stat._reg);

    stat.setMode(StatusCode::Mode::Seeking);

    CDROM_interrupt.push_back(2);
    writeResponse(stat._reg);

    stat.setMode(StatusCode::Mode::None);
}

void CDROM::cmdGetlocP() {
    uint32_t tmp = readSector + 2 * 75;
    uint32_t minute = tmp / 75 / 60;
    uint32_t second = (tmp / 75) % 60;
    uint32_t sector = tmp % 75;

    CDROM_interrupt.push_back(3);
    writeResponse(0x01);                // track
    writeResponse(0x01);                // index
    writeResponse(bcd::toBcd(minute));  // minute (track)
    writeResponse(bcd::toBcd(second));  // second (track)
    writeResponse(bcd::toBcd(sector));  // sector (track)
    writeResponse(bcd::toBcd(minute));  // minute (disc)
    writeResponse(bcd::toBcd(second));  // second (disc)
    writeResponse(bcd::toBcd(sector));  // sector (disc)
}

void CDROM::cmdGetTN() {
    CDROM_interrupt.push_back(3);
    writeResponse(stat._reg);
    writeResponse(0x01);
    writeResponse(cue.getTrackCount());
}

void CDROM::cmdGetTD() {
    int track = readParam();
    CDROM_interrupt.push_back(3);
    writeResponse(stat._reg);
    if (track == 0)  // end of last track
    {
        auto diskSize = cue.getDiskSize();
        printf("GetTD(0): minute: %d, second: %d\n", diskSize.mm, diskSize.ss);

        writeResponse(bcd::toBcd(diskSize.mm));
        writeResponse(bcd::toBcd(diskSize.ss));
    } else {  // Start of n track
        if (track > cue.getTrackCount()) {
            // Error
            return;
        }

        auto start = cue.tracks.at(track - 1).start;
        printf("GetTD(%d): minute: %d, second: %d\n", track, start.mm, start.ss);
        writeResponse(bcd::toBcd(start.mm));
        writeResponse(bcd::toBcd(start.ss));
    }
}

void CDROM::cmdSeekL() {
    ((mips::CPU*)_cpu)->dma->dma3.seekTo(readSector);

    CDROM_interrupt.push_back(3);
    writeResponse(stat._reg);

    stat.setMode(StatusCode::Mode::Seeking);

    CDROM_interrupt.push_back(2);
    writeResponse(stat._reg);

    status.dataFifoEmpty = 0;
}

void CDROM::cmdTest() {
    uint8_t opcode = readParam();
    if (opcode == 0x20)  // Get CDROM BIOS date/version (yy,mm,dd,ver)
    {
        CDROM_interrupt.push_back(3);
        writeResponse(0x97);
        writeResponse(0x01);
        writeResponse(0x10);
        writeResponse(0xc2);
    } else if (opcode == 0x03) {  // Force motor off, used in swap
        stat.motor = 0;
        CDROM_interrupt.push_back(3);
        writeResponse(stat._reg);
    } else {
        printf("Unimplemented test CDROM opcode (0x%x)!\n", opcode);
    }
}

void CDROM::cmdGetId() {
    CDROM_interrupt.push_back(3);
    writeResponse(stat._reg);

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

void CDROM::cmdReadS() {
    stat.setMode(StatusCode::Mode::Reading);

    CDROM_interrupt.push_back(3);
    writeResponse(stat._reg);
}

void CDROM::cmdReadTOC() {
    CDROM_interrupt.push_back(3);
    writeResponse(stat._reg);

    CDROM_interrupt.push_back(2);
    writeResponse(stat._reg);
}

void CDROM::cmdUnlock() {
    // Semi implemented
    CDROM_interrupt.push_back(5);
    writeResponse(0x11);
    writeResponse(0x40);
}

void CDROM::handleCommand(uint8_t cmd) {
    if (verbose) {
        printf("CDROM  COMMAND: 0x%02x", cmd);
        if (!CDROM_params.empty()) {
            putchar('(');
            bool first = true;
            for (auto p : CDROM_params) {
                if (!first) printf(", ");
                printf("0x%02x", p);
                first = false;
            }
            putchar(')');
        }
        printf("\n");
    }

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
                ((mips::CPU*)_cpu)->dma->dma3.advanceSector();
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
    ((mips::CPU*)_cpu)->state = mips::CPU::State::pause;
}
}
}
