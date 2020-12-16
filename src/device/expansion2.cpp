#include "expansion2.h"
#include <cstdio>

Expansion2::Expansion2() { reset(); }

void Expansion2::reset() { post = 0; }

uint8_t Expansion2::read(uint32_t address) {
    (void)address;
    return 0;
}

void Expansion2::write(uint32_t address, uint8_t data) {
    if (address == 0x41) {
        post = data;
    }
    // PCSX-Redux/Openbios stdout channel
    if (address == 0x80) {
        putchar(data);
    }
}