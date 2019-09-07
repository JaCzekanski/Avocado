#include "memory_control.h"

MemoryControl::MemoryControl() { reset(); }

void MemoryControl::reset() {}

uint32_t MemoryControl::read(uint32_t address) {
    (void)address;
    return 0;
}

void MemoryControl::write(uint32_t address, uint32_t data) {
    (void)address;
    (void)data;
}