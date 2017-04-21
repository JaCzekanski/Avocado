#include "cdrom.h"
#include "../mips.h"
#include <cstdio>
#include <cassert>
#include "../utils/bcd.h"

namespace device {
namespace cdrom {
CDROM::CDROM() {}

void CDROM::step() {
    status.transmissionBusy = 0;
    if (!CDROM_interrupt.empty()) {
        if ((interruptEnable & 7) & (CDROM_interrupt.front() & 7)) {
            ((mips::CPU*)_cpu)->interrupt->IRQ(interrupt::CDROM);
        }
    }
}

uint8_t CDROM::read(uint32_t address) {
    if (address == 0) {  // CD Status
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
        return response;
    }
    if (address == 2) {  // CD Data
        printf("UNIMPLEMENTED CDROM READ!\n");
        //((mips::CPU*)_cpu)->state = mips::CPU::State::pause;
        return 0;
    }
    if (address == 3) {                                // CD Interrupt enable / flags
        if (status.index == 0 || status.index == 2) {  // Interrupt enable
            return interruptEnable;
        }
        if (status.index == 1 || status.index == 3) {  // Interrupt flags
            uint8_t _status = 0b11100000;
            if (!CDROM_interrupt.empty()) {
                _status |= CDROM_interrupt.front() & 7;
            }
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
    uint8_t minute = bcdToBinary(readParam());
    uint8_t second = bcdToBinary(readParam());
    uint8_t sector = bcdToBinary(readParam());
    printf("@ 0x%08x ", ((mips::CPU*)_cpu)->PC);
    printf("Setloc: min: %d  sec: %d  sect: %d\n", minute, second, sector);

    readSector = sector + (second * 75) + (minute * 60 * 75);
    readSector -= 2 * 75;
    assert(readSector >= 0);
    ((mips::CPU*)_cpu)->dma->dma3.seekTo(readSector);

    CDROM_interrupt.push_back(3);
    writeResponse(stat._reg);
}

void CDROM::cmdPlay() {
    // Play NOT IMPLEMENTED
    // int track = readParam();
    // param or setloc used
    stat.setMode(StatusCode::Mode::Playing);

    CDROM_interrupt.push_back(3);
    writeResponse(stat._reg);
}

void CDROM::cmdReadN() {
    status.dataFifoEmpty = 1;
    stat.setMode(StatusCode::Mode::Reading);

    CDROM_interrupt.push_back(3);
    writeResponse(stat._reg);

    CDROM_interrupt.push_back(1);
    writeResponse(stat._reg);
}

void CDROM::cmdPause() {
    status.dataFifoEmpty = 0;

    CDROM_interrupt.push_back(3);
    writeResponse(stat._reg);

    stat.setMode(StatusCode::Mode::None);

    CDROM_interrupt.push_back(2);
    writeResponse(stat._reg);

    status.dataFifoEmpty = 0;  // ?
}

void CDROM::cmdInit() {
    CDROM_interrupt.push_back(3);
    writeResponse(stat._reg);

    stat.motor = 1;
    stat.setMode(StatusCode::Mode::None);

    CDROM_interrupt.push_back(2);
    writeResponse(stat._reg);
}

void CDROM::cmdDemute() {
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

void CDROM::cmdGetTN() {
    CDROM_interrupt.push_back(3);
    writeResponse(stat._reg);
    writeResponse(0x01);
    writeResponse(0x01);
}

void CDROM::cmdGetTD() {
    int index = readParam();
    CDROM_interrupt.push_back(3);
    writeResponse(stat._reg);
    if (index == 0)  // end of last track
    {
        // get size (25B3 AF20)
        // divide by 2352 (4 1A86)
        // x / 60 / 75 - minute
        // (x % (60 * 75) / 75) + 2 - second
        int x = ((mips::CPU*)_cpu)->dma->dma3.getIsoSize();
        x /= 2352;
        int minute = x / 60 / 75;
        int second = ((x % (60 * 75)) / 75) + 2;
        printf("GetTD: minute: %d, second: %d", minute, second);

        writeResponse(((minute / 10) << 4) | (minute % 10));
        writeResponse(((second / 10) << 4) | (second % 10));
    }
    if (index == 1) {
        writeResponse(0x00);
        writeResponse(0x02);
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
    status.dataFifoEmpty = 1;
    stat.setMode(StatusCode::Mode::Reading);

    CDROM_interrupt.push_back(3);
    writeResponse(stat._reg);

    CDROM_interrupt.push_back(1);
    writeResponse(stat._reg);
}

void CDROM::cmdReadTOC() {
    stat.setMode(StatusCode::Mode::Reading);

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

    CDROM_interrupt.clear();
    if (cmd == 0x01)
        cmdGetstat();
    else if (cmd == 0x02)
        cmdSetloc();
    else if (cmd == 0x03)
        cmdPlay();
    else if (cmd == 0x06)
        cmdReadN();
    else if (cmd == 0x1b)
        cmdReadS();
    else if (cmd == 0x0e)
        cmdSetmode();
    else if (cmd == 0x09)
        cmdPause();
    else if (cmd == 0x0c)
        cmdDemute();
    else if (cmd == 0x13)
        cmdGetTN();
    else if (cmd == 0x14)
        cmdGetTD();
    else if (cmd == 0x15)
        cmdSeekL();
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
        if (data & 0x80) {                    // want data
            CDROM_interrupt.push_back(1);
            writeResponse(stat._reg);
        } else {  // clear data fifo
            status.dataFifoEmpty = true;
        }
        return;
    }
    if (address == 2 && status.index == 1) {  // Interrupt enable
        interruptEnable = data;
        return;
    }
    if (address == 3 && status.index == 1) {  // Interrupt flags
        if (data & 0x40)                      // reset parameter fifo
        {
            CDROM_params.clear();
            status.parameterFifoEmpty = 1;
            status.parameterFifoFull = 1;
        }

        data &= 0x1f;
        if ((data == 0x07 || data == 0x1f) && !CDROM_interrupt.empty()) CDROM_interrupt.pop_front();

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
