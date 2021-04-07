#include "interrupt.h"
#include "system.h"

using namespace interrupt;

Interrupt::Interrupt(System* sys) : sys(sys) { reset(); }

void Interrupt::reset() {
    // TODO: Verify with real HW
    status._reg = 0;
    mask._reg = 0;
}

void Interrupt::trigger(IrqNumber irq) {
    if (irq > 10) return;
    status._reg |= (1 << irq);
    step();
}

bool Interrupt::interruptPending() { return (status._reg & mask._reg) ? true : false; }

void Interrupt::step() {
    // notify cop0
    sys->cpu->cop0.cause.interruptPending = interruptPending() ? 4 : 0;
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