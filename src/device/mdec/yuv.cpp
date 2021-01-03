#include "mdec.h"
#include "utils/math.h"

namespace mdec {
// Helpers for accessing 1d arrays with 2d addressing
#define _CR ((int16_t(*)[8])crblk.data())
#define _CB ((int16_t(*)[8])cbblk.data())
#define _Y ((int16_t(*)[8])yblk.data())

DecodedBlock MDEC::yuvToRgb(Status::CurrentBlock currentBlock) {
    // YUV 4:2:0
    // Y component is at full resolution
    // Cr and Cb components are half resolution horizontally and vertically

    const int mx = ((int)currentBlock & 0b01) * 8;
    const int my = (((int)currentBlock & 0b10) >> 1) * 8;

    // Y, Cb, Cr
    auto sample = [&](int x, int y) -> std::tuple<int16_t, int16_t, int16_t> {
        int16_t Y = _Y[y][x];
        int16_t Cb = _CB[(my + y) / 2][(mx + x) / 2];
        int16_t Cr = _CR[(my + y) / 2][(mx + x) / 2];
        return std::make_tuple(Y, Cb, Cr);
    };

    DecodedBlock block;
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            auto [Y, Cb, Cr] = sample(x, y);
            // Y += 128;

            // Constants from https://en.wikipedia.org/wiki/YCbCr#JPEG_conversion
            int R = Y + (1.402 * (Cr));
            int G = Y - (0.334136 * (Cb)) - (0.714136 * (Cr));
            int B = Y + (1.772 * (Cb));

            uint8_t r = clamp(R + 128, 0, 255);
            uint8_t g = clamp(G + 128, 0, 255);
            uint8_t b = clamp(B + 128, 0, 255);
            // uint8_t r = clamp<int16_t>(Y, 0, 255);
            // uint8_t g = clamp<int16_t>(Y, 0, 255);
            // uint8_t b = clamp<int16_t>(Y, 0, 255);

            // TODO: unsigned;
            block[y * 8 + x] = (b << 16) | (g << 8) | (r);
        }
    }
    return block;
}
}  // namespace mdec