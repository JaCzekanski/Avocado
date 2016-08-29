#include "controller.h"
#include "../mips.h"
#include <stdlib.h>

namespace device {
namespace controller {
void Controller::handleByte(uint8_t byte) {
    static bool selected = false;
    if (byte == 0x01 && !selected) {
        fifo.push_back(0xff);
        selected = true;
    }
    if (byte == 0x42 && selected) {
        fifo.push_back(0x41);
        fifo.push_back(0x5a);
        fifo.push_back(~state._byte[0]);
        fifo.push_back(~state._byte[1]);

        printf("JOYPAD --> 0x%04x\n", state._reg);
        selected = false;
    }
}

Controller::Controller() {}

void Controller::step() {}

uint8_t Controller::read(uint32_t address) {
    if (address == 0 && !fifo.empty()) {  // RX

        if (!fifo.empty()) {
            ((mips::CPU*)_cpu)->interrupt->IRQ(7);
        }
        uint8_t value = fifo.front();
        fifo.pop_front();

        return value;
    }
    if (address == 4) {             // STAT
        return rand() | (1 << 0) |  // must be set
               ((!fifo.empty()) << 1) | (1 << 2) | (1 << 7);
    }
    if (address >= 8 && address < 10) {  // MODE
        return mode >> ((address - 8) * 8);
    }
    if (address >= 10 && address < 12) {  // CTRL
        return control >> ((address - 10) * 8);
    }
    if (address >= 14 && address < 16) {  // BAUD
        return baud >> ((address - 14) * 8);
    }
    return 0;
}

void Controller::write(uint32_t address, uint8_t data) {
    if (address == 0) {  // TX
        handleByte(data);
        return;
    }

    if (address >= 8 && address < 10) {  // MODE
        mode &= 0xff << (address - 8);
        mode |= data << (address - 8);
        return;
    }
    if (address >= 10 && address < 12) {  // CTRL
        control &= 0xff << (address - 10);
        control |= data << (address - 10);

        // handle bit4
        if (control & (1 << 4)) {
            // clear bit 3 and 9 in stat
        }
        return;
    }
    if (address >= 14 && address < 16) {  // BAUD
        baud &= 0xff << (address - 14);
        baud |= data << (address - 14);
        return;
    }
}
}
}
