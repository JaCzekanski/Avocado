#include "mdec.h"
#include "utils/math.h"

namespace mdec {
// Helpers for accessing 1d arrays with 2d addressing
#define _CR ((int16_t(*)[8])crblk.data())
#define _CB ((int16_t(*)[8])cbblk.data())
#define _Y(x) ((int16_t(*)[8])yblk[x].data())

void MDEC::yuvToRgb(decodedBlock& output, int blockX, int blockY) {
    // YUV 4:2:0
    // Y component is at full resolution
    // Cr and Cb components are half resolution horizontally and vertically

    // Y, Cb, Cr
    auto sample = [this](int x, int y, int yBlock) -> std::tuple<int16_t, int16_t, int16_t> {
        int16_t Y = _Y(yBlock)[y % 8][x % 8];
        int16_t Cb = _CB[y / 2][x / 2];
        int16_t Cr = _CR[y / 2][x / 2];
        return std::make_tuple(Y, Cb, Cr);
    };

    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            int yBlock = (blockY / 8) * 2 + (blockX / 8);
            auto [Y, Cb, Cr] = sample(x + blockX, y + blockY, yBlock);
            // int16_t Y = yblk[(blockY/8)*2 * (blockX/8)].at(y*8 + x);
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
            output[(blockY + y) * 16 + (blockX + x)] = (b << 16) | (g << 8) | (r);
        }
    }
}
}  // namespace mdec