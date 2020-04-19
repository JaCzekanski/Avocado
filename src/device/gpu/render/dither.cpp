#include "dither.h"

namespace {
constexpr int8_t ditherTable[4][4] = {
    {-4, +0, -3, +1},  //
    {+2, -2, +3, -1},  //
    {-3, +1, -4, +0},  //
    {+3, -1, +2, -2}   //
};

constexpr dither_lut_t generateDitherLUT() {
    dither_lut_t lut = {};
    const auto clamp = [](int c) {
        if (c < 0) {
            return 0;
        } else if (c > 255) {
            return 255;
        } else {
            return c;
        }
    };

    for (int y = 0; y < 4; y++) {
        for (int x = 0; x < 4; x++) {
            for (int c = 0; c < 256; c++) {
                lut[y][x][c] = clamp(ditherTable[y][x] + c);
            }
        }
    }
    return lut;
}
};  // namespace

const dither_lut_t ditherLUT = generateDitherLUT();