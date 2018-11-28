#include <tuple>
#include "mdec.h"
#include "utils/logic.h"
#include "utils/math.h"

namespace mdec {
std::array<int16_t, 64> crblk = {{0}};
std::array<int16_t, 64> cbblk = {{0}};
std::array<int16_t, 64> yblk[4] = {{0}};

// Helpers for accessing 1d arrays with 2d addressing
#define _CR ((int16_t(*)[8])crblk.data())
#define _CB ((int16_t(*)[8])cbblk.data())
#define _Y(x) ((int16_t(*)[8])yblk[x].data())

const std::array<uint8_t, 64> zigzag = {{
    0,  1,  5,  6,  14, 15, 27, 28,  //
    2,  4,  7,  13, 16, 26, 29, 42,  //
    3,  8,  12, 17, 25, 30, 41, 43,  //
    9,  11, 18, 24, 31, 40, 44, 53,  //
    10, 19, 23, 32, 39, 45, 52, 54,  //
    20, 22, 33, 38, 46, 51, 55, 60,  //
    21, 34, 37, 47, 50, 56, 59, 61,  //
    35, 36, 48, 49, 57, 58, 62, 63   //
}};

constexpr std::array<uint8_t, 64> generateZagzig() {
    std::array<uint8_t, 64> zagzig = {{0}};
    for (int i = 0; i < 64; i++) {
        zagzig[zigzag[i]] = i;
    }
    return zagzig;
}

const std::array<uint8_t, 64> zagzig = generateZagzig();

void yuvToRgb(uint32_t* output, int blockX, int blockY) {
    // YUV 4:2:0
    // Y component is at full resolution
    // Cr and Cb components are half resolution horizontally and vertically

    // Y, Cb, Cr
    auto sample = [](int x, int y, int yBlock) -> std::tuple<int16_t, int16_t, int16_t> {
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
void MDEC::decodeMacroblocks() {
    uint16_t* src = input.data();

    int block;
    for (block = 0;; block++) {
        output.resize(output.size() + 16 * 16);

        uint32_t* out = output.data() + block * 16 * 16;
        decodeMacroblock(src, out);

        if (src >= input.data() + input.size() * 2) {
            break;
        }
    }
    printf("decoded %d blocks\n", block);
}

void MDEC::decodeMacroblock(uint16_t*& src, uint32_t* out) {
    decodeBlock(crblk, src, colorQuantTable);
    decodeBlock(cbblk, src, colorQuantTable);

    decodeBlock(yblk[0], src, luminanceQuantTable);
    decodeBlock(yblk[1], src, luminanceQuantTable);
    decodeBlock(yblk[2], src, luminanceQuantTable);
    decodeBlock(yblk[3], src, luminanceQuantTable);

    yuvToRgb(out, 0, 0);
    yuvToRgb(out, 0, 8);
    yuvToRgb(out, 8, 0);
    yuvToRgb(out, 8, 8);

    // Handle format conversion
}

// First byte in block
union DCT {
    struct {
        uint16_t dc : 10;      // Direct Current
        uint16_t qFactor : 6;  // Quantization factor
    };
    uint16_t _;
    DCT(uint16_t val) : _(val){};
};

// compressed data
union RLE {
    struct {
        uint16_t ac : 10;     // relative value
        uint16_t zeroes : 6;  // bytes to skip
    };
    uint16_t _;
    RLE(uint16_t val) : _(val){};
};

void MDEC::decodeBlock(std::array<int16_t, 64>& blk, uint16_t*& src, const std::array<uint8_t, 64>& table) {
    blk.fill(0);

    // Block structure:
    // (optional) 0xfe00 padding
    // DCT - 1x 16bit
    // RLE compressed data - 0-63x 16bit
    // (optional) End of block (0xfe00)

    // TODO: Range check
    while (*src == 0xfe00) {
        src++;  // Skip padding
    }

    DCT dct = *src++;
    int32_t current = extend_sign<9>(dct.dc);

    int32_t value = current * table[0];

    for (int n = 0; n < 64;) {
        if (dct.qFactor == 0) {
            value = current * 2;
        }

        value = clamp<int32_t>(value, -0x400, 0x3ff);

        if (dct.qFactor > 0) {
            blk.at(zagzig[n]) = value;
        } else if (dct.qFactor == 0) {
            blk.at(n) = value;
        }

        RLE rle = *src++;
        current = extend_sign<9>(rle.ac);
        n += rle.zeroes + 1;

        value = (current * table[n] * dct.qFactor + 4) / 8;
    }

    idct(blk);
}

void MDEC::idct(std::array<int16_t, 64>& src) {
    // std::array<int16_t, 64> dst = {{}};
    std::array<int64_t, 64> tmp = {{}};

    for (int x = 0; x < 8; x++) {
        for (int y = 0; y < 8; y++) {
            int64_t sum = 0;
            for (int i = 0; i < 8; i++) {
                sum += idctTable.at(i * 8 + y) * src.at(x + i * 8);
            }
            tmp.at(x + y * 8) = sum;
        }
    }

    for (int x = 0; x < 8; x++) {
        for (int y = 0; y < 8; y++) {
            int64_t sum = 0;
            for (int i = 0; i < 8; i++) {
                sum += tmp.at(i + y * 8) * idctTable.at(x + i * 8);
            }

            int round = (sum >> 31) & 1;
            src.at(x + y * 8) = (sum >> 32) + round;
        }
    }
}

};  // namespace mdec
