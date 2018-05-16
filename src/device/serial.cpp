#include "serial.h"
#include <cstdio>

Serial::Serial() { reset(); }

void Serial::step() {}

void Serial::reset() { status._reg = 0x00000005; }

uint8_t Serial::read(uint32_t address) {
    if (address >= 4 && address < 8) {
        return status._reg;
    }

    // printf("Serial read @ 0x%02x\n", address);
    return 0;
}

void Serial::write(uint32_t address, uint8_t data) {
    // printf("Serial write @ 0x%02x: 0x%02x\n", address, data);
}