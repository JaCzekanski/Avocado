#include "bcd.h"

int bcdToBinary(int bcd) {
    int hi = (bcd >> 4) & 0xf;
    int lo = bcd & 0xf;

    return hi * 10 + lo;
}