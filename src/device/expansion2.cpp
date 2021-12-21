#include "expansion2.h"
#include "config.h"
#include <cstdio>

Expansion2::Expansion2() { reset(); }

void Expansion2::reset() { post = 0; }

uint8_t Expansion2::read(uint32_t address) {
    if (address == 0x21) {  // DUART Status
        return 1 << 2;      // Tx Empty
    }
    return 0;
}

void Expansion2::write(uint32_t address, uint8_t data) {
    if (address == 0x22) {  // DUART Command
        // Ignore (Enable Tx/Rx, reset/flush commands)
    } else if (address == 0x23) {  // DUART Tx
        if (config.debug.log.system) {
            putchar(data);
        }
    } else if (address == 0x24) {  // DUART Aux Control
        // Ignore (baud rate, int flags)
    } else if (address == 0x41) {
        post = data;
    } else if (address == 0x80) {  // PCSX-Redux/Openbios stdout channel
        if (config.debug.log.system) {
            putchar(data);
        }
    }
}