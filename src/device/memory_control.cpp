#include "memory_control.h"
#include <cstdio>

MemoryControl::MemoryControl() { reset(); }

void MemoryControl::reset() {}

uint32_t MemoryControl::read(uint32_t address) {
    // printf("R Unhandled memory control\n");
    return 0;
}

void MemoryControl::write(uint32_t address, uint32_t data) {
    // printf("W Unhandled memory control\n");
}