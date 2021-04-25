#include "ram_control.h"
#include <fmt/core.h>
#include <config.h>

RamControl::RamControl() {
    verbose = config.debug.log.memoryControl > 0;
    reset();
}

void RamControl::reset() {
    ramSize._reg = 0x00000b88;  // TODO: Check reset value
}

uint8_t RamControl::read(uint32_t address) { return ramSize.read(address); }

void RamControl::write(uint32_t address, uint8_t data) {
    ramSize.write(address, data);
    if (verbose && address == 0x3) {
        fmt::print("[RAMCTRL] RAM_SIZE: 0x{:08x}\n", ramSize._reg);
    }
}