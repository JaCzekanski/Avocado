#pragma once
#include <cstdint>

const inline int8_t ditherTable[4][4] = {
    {-4, +0, -3, +1},  //
    {+2, -2, +3, -1},  //
    {-3, +1, -4, +0},  //
    {+3, -1, +2, -2}   //
};