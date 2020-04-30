#include "serial.h"
#include <fmt/core.h>

Serial::Serial() { reset(); }

void Serial::step() {}

void Serial::reset() { status._reg = 0xffff; }

uint8_t Serial::read(uint32_t address) {
    if (address == 0) {
        fmt::print("[SERIAL] Rx: ?\n");

        return '?';
    }
    if (address >= 4 && address < 8) {
        return status._byte[address - 4];
    }
    if (address >= 10 && address < 12) {
        return control._byte[address - 10];
    }
    if (address >= 14 && address < 16) {
        return baud._byte[address - 14];
    }

    fmt::print("[SERIAL] read @ 0x{:02x}\n", address);
    return 0;
}

void Serial::write(uint32_t address, uint8_t data) {
    if (address == 0) {
        fmt::print("[SERIAL] TX: 0x{:02x}\n", data);
        return;
    }
    if (address >= 10 && address < 12) {
        control._byte[address - 10] = data;
        return;
    }
    if (address >= 14 && address < 16) {
        baud._byte[address - 14] = data;
        return;
    }
    fmt::print("[SERIAL] write @ 0x{:02x}: 0x{:02x}\n", address, data);
}