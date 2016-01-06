#include "cdrom.h"
#include "../mips.h"
#include <cstdio>

namespace device {
namespace cdrom {
CDROM::CDROM() {}

void CDROM::step() {}

uint8_t CDROM::read(uint32_t address) {
    if (address == 0) {
        uint8_t status = CDROM_index;
        // status |= (!CDROM_data.emprt()) << 6;
        status |= (!CDROM_response.empty()) << 5;
        status |= (CDROM_params.size() >= 16) << 4;
        status |= (CDROM_params.empty()) << 3;

        return status;
    }
    if (address == 1) {
        uint8_t response = 0;
        if (!CDROM_response.empty()) {
            response = CDROM_response.front();
            CDROM_response.pop_front();
        }
        return response;
    }
    if (address == 3) {  // type of response received
        uint8_t status = 0xe0;
        if (!CDROM_interrupt.empty()) {
            status |= CDROM_interrupt.front();
            CDROM_interrupt.pop_front();
        }

        return status;
    }
    return 0;
}

void CDROM::write(uint32_t address, uint8_t data) {
    if (address == 0) CDROM_index = data & 3;
    if (address == 1) {  // Command register
        CDROM_interrupt.clear();
        CDROM_response.clear();
        if (CDROM_index == 0) {
            if (data == 0x0a)  // Init
            {
                CDROM_interrupt.push_back(3);
                CDROM_response.push_back(0x20);  // stat
                CDROM_interrupt.push_back(2);
                CDROM_response.push_back(0x20);  // stat
            }
            if (data == 0x19)  // Test
            {
                if (CDROM_params.front() == 0x20)  // Get CDROM BIOS date/version (yy,mm,dd,ver)
                {
                    CDROM_params.pop_front();
                    CDROM_interrupt.push_back(3);
                    CDROM_response.push_back(0x97);
                    CDROM_response.push_back(0x01);
                    CDROM_response.push_back(0x10);
                    CDROM_response.push_back(0xc2);
                }
            }
        }
        CDROM_params.clear();
    }
    if (address == 2) {  // Parameter fifo
        if (CDROM_index == 0) {
            CDROM_params.push_back(data);
        }
    }
}
}
}