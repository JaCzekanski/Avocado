#include "interrupt.h"
#include "../mips.h"
#include <cstdio>

namespace device {
namespace interrupt {
Interrupt::Interrupt() {}

void Interrupt::step() {
    // notify cop0
    mips::CPU *cpu = (mips::CPU *)_cpu;
    cpu->cop0.cause.interruptPending = interruptPending() ? 4 : 0;
}

uint8_t Interrupt::read(uint32_t address) {
    if (address < 0x2) return status._byte[address];
    if (address >= 0x4 && address < 0x6) return mask._byte[address - 4];
    return 0;
}

void Interrupt::write(uint32_t address, uint8_t data) {
    if (address < 0x2) {
        status._byte[address] &= data;  // write 0 to ACK
    }
    if (address >= 0x4 && address < 0x6) mask._byte[address - 4] = data;

    step();
}
}
}
