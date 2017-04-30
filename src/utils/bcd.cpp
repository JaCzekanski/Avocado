#include "bcd.h"

namespace bcd {
uint8_t toBinary(uint8_t bcd) {
    int hi = (bcd >> 4) & 0xf;
    int lo = bcd & 0xf;

    return hi * 10 + lo;
}

uint8_t toBcd(uint8_t b) { return ((b / 10) << 4) | (b % 10); }
}
