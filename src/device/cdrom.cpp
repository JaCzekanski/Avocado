#include "cdrom.h"
#include "../mips.h"
#include <cstdio>

namespace device {
namespace cdrom {
CDROM::CDROM() {}

void CDROM::step() {
    if (!CDROM_interrupt.empty()) {
        status.transmissionBusy = 1;
        ((mips::CPU*)_cpu)->interrupt->IRQ(2);
    } else {
        status.transmissionBusy = 0;
    }
}

uint8_t CDROM::read(uint32_t address) {
    if (address == 0) {
        //        uint8_t status = CDROM_index;
        //		status |= (1) << 6;
        //        status |= (!CDROM_response.empty()) << 5;
        //        status |= (!(CDROM_params.size() >= 16)) << 4;
        //        status |= (CDROM_params.empty()) << 3;
        //		status |= 0 << 2; // XA-ADPCM empty

        printf("CDROM%d.%d->R STATUS: 0x%02x\n", address, status.index, status._reg);
        return status._reg;
    }
    if (address == 1) {
        uint8_t response = 0;
        if (!CDROM_response.empty()) {
            response = CDROM_response.front();
            CDROM_response.pop_front();

            if (CDROM_response.empty()) {
                status.responseFifoEmpty = 0;
                // status.dataFifoEmpty = 0;
            }
            //
            //            if (!CDROM_interrupt.empty()) {
            //               ((mips::CPU*)_cpu)->interrupt->IRQ(2);
            //            }
        }

        printf("CDROM%d.%d->R   RESP: 0x%02x\n", address, status.index, response);
        return response;
    }
    if (address == 3) {  // type of response received
        if (status.index == 1 || status.index == 3) {
            uint8_t _status = 0b11100000;
            if (!CDROM_interrupt.empty()) {
                int interr = CDROM_interrupt.front();
                _status |= interr & 7;
                //((mips::CPU*)_cpu)->interrupt->IRQ(2);
            }

            printf("CDROM%d.%d->R    INT: 0x%02x\n", address, status.index, _status);
            return _status;
        }
    }
    // printf("CDROM%d.%d->R    ?????\n", address, status.index);
    return 0;
}

void CDROM::write(uint32_t address, uint8_t data) {
    if (address == 0) {
        status.index = data & 3;
        return;
    }
    if (address == 1) {  // Command register
        if (status.index == 0) {
            printf("CDROM%d.%d<-W    CMD: 0x%02x", address, status.index, data);
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
            if (data == 0x01)  // Getstat
            {
                CDROM_interrupt.push_back(3);
                writeResponse(0b00000010 | (shellOpen << 4));
            } else if (data == 0x02)  // Setloc
            {
                uint8_t minute = readParam();
                uint8_t second = readParam();
                uint8_t sector = readParam();
                printf("Setloc: min: %d  sec: %d  sect: %d\n", minute, second, sector);

                // sect size: 2352
                if (second >= 2) second -= 2;
                readSector = sector + (second * 75) + (minute * 60 * 75);
                if (readSector < 0) readSector = 0;

                CDROM_interrupt.push_back(3);
                writeResponse(0b00000010);
            } else if (data == 0x06)  // ReadN
            {
                status.dataFifoEmpty = 1;
                CDROM_interrupt.push_back(3);
                writeResponse(0b00000010);

                for (int i = 0; i < 10; i++) {
                    CDROM_interrupt.push_back(1);
                    writeResponse(0b00100010);
                }
            } else if (data == 0x0e)  // Setmode
            {
                CDROM_interrupt.push_back(3);
                writeResponse(0b00000010);
            } else if (data == 0x09)  // Pause
            {
                CDROM_interrupt.push_back(3);
                writeResponse(0b00100010);

                CDROM_interrupt.push_back(2);
                writeResponse(0b00000010);

                status.dataFifoEmpty = 0;  // ?
            } else if (data == 0x0c)       // Demute
            {
                CDROM_interrupt.push_back(3);
                writeResponse(0b00000010);
            } else if (data == 0x13)  // GetTN
            {
                CDROM_interrupt.push_back(3);
                writeResponse(0b01000010);
                writeResponse(1);
                writeResponse(9);
            } else if (data == 0x15)  // SeekL
            {
                int sectorSize = 2352;
                fseek(((mips::CPU*)_cpu)->dma->dma3.f, readSector * sectorSize + 24, SEEK_SET);

                CDROM_interrupt.push_back(3);
                writeResponse(0b00000010);

                CDROM_interrupt.push_back(2);
                writeResponse(0b00000010);

                status.dataFifoEmpty = 0;
            } else if (data == 0x0a)  // Init
            {
                CDROM_interrupt.push_back(3);
                writeResponse(0b00000010);  // stat
                CDROM_interrupt.push_back(2);
                writeResponse(0b00000010);  // stat
            } else if (data == 0x19)        // Test
            {
                if (readParam() == 0x20)  // Get CDROM BIOS date/version (yy,mm,dd,ver)
                {
                    CDROM_interrupt.push_back(3);
                    writeResponse(0x97);
                    writeResponse(0x01);
                    writeResponse(0x10);
                    writeResponse(0xc2);
                }
            } else if (data == 0x1A)  // GetId
            {
                CDROM_interrupt.push_back(3);
                writeResponse(0b00100010);

                CDROM_interrupt.push_back(2);
                writeResponse(0x02);
                writeResponse(0x00);
                writeResponse(0x20);
                writeResponse(0x00);
                writeResponse('S');
                writeResponse('C');
                writeResponse('E');
                writeResponse('A');   // 0x45 E, 0x41 A, 0x49 I
            } else if (data == 0x1e)  // ReadTOC
            {
                CDROM_interrupt.push_back(3);
                writeResponse(0b00000010);

                CDROM_interrupt.push_back(2);
                writeResponse(0b00000010);
            } else {
                printf("Unimplemented!");
            }
            //((mips::CPU*)_cpu)->interrupt->IRQ(2);
            CDROM_params.clear();
            status.parameterFifoEmpty = 1;
            status.parameterFifoFull = 1;
            status.transmissionBusy = 1;
            status.xaFifoEmpty = 0;

        } else {
            // printf("CDROM%d.%d<-W    ???: 0x%02x\n", address, status.index, data);
        }
        return;
    }
    if (address == 2) {  // Parameter fifo
        if (status.index == 0) {
            CDROM_params.push_back(data);
            status.parameterFifoEmpty = 0;
            status.parameterFifoFull = (CDROM_params.size() >= 16);

            // printf("CDROM%d.%d<-W  PARAM: 0x%02x\n", address, status.index, data);
            return;
        }
    }
    if (address == 3) {
        if (status.index == 1) {  // Interrupt Flag Register R/W
            if ((data & 0x7) == 0x07 && !CDROM_interrupt.empty()) CDROM_interrupt.pop_front();
            // CDROM_params.push_back(data);

            if (data & 0x40)  // reset parameter fifo
            {
                CDROM_params.clear();
            }

            printf("CDROM%d.%d<-W  INTE   0x%02x\n", address, status.index, data);
            return;
        }
    }
    printf("CDROM%d.%d<-W         0x%02x\n", address, status.index, data);
}
}
}