#include "mdec.h"
#include "../mips.h"
#include <cassert>
#include <cstring>

namespace device {
namespace mdec {

MDEC::MDEC() { reset(); }

void MDEC::step() {}

void MDEC::reset() {
    command._reg = 0;
    status._reg = 0x80040000;
}

uint8_t MDEC::read(uint32_t address) {
    // printf("MDEC read @ 0x%02x\n", address);
    if (address < 4) {
        return data.read(address);
    }
    if (address >= 4 && address < 8) {
        return status.read(address - 4);
    }

    assert(false && "UNHANDLED MDEC READ");
    return 0;
}

void MDEC::handleCommand(uint8_t cmd, uint32_t data) {
    this->cmd = cmd;
    switch (cmd) {
        case 1:
            break;
        case 2:  // Set Quant table
            color = data & 1;
            // 64 quant table when luma only, 64+64 when color
            if (!color)
                paramCount = 64;
            else
                paramCount = 128;
            status.setBit(29, 1);
            break;
    }
}

void MDEC::write(uint32_t address, uint8_t data) {
    printf("MDEC write @ 0x%02x: 0x%02x\n", address, data);
    if (address < 4) {
        command.write(address, data);
        if (address == 3) {
            if (paramCount == 0)
                handleCommand((command._reg >> 29) & 0x07, command._reg & 0x1FFFFFFF);
            else {
                paramCount--;
                status._reg &= 0xffff0000;
                status._reg |= (paramCount - 1) & 0xffff;
            }
        }
        return;
    }
    if (address >= 4 && address < 8) {
        _control.write(address, data);
        if (address == 7) {
            if (_control.getBit(31)) reset();
            status.setBit(30, _control.getBit(28));
            status.setBit(29, _control.getBit(27));
        }
        return;
    }

    assert(false && "UNHANDLED MDEC WRITE");
}
}
}
