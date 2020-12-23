#include "mdec.h"
#include "tables.h"
#include "utils/logic.h"
#include "utils/math.h"

namespace mdec {
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

bool MDEC::decodeBlock(std::array<int16_t, 64>& blk, const std::vector<uint16_t>& input, const std::array<uint8_t, 64>& table) {
    blk.fill(0);

    // Block structure:
    // (optional) 0xfe00 padding
    // DCT - 1x 16bit
    // RLE compressed data - 0-63x 16bit
    // (optional) End of block (0xfe00)

    auto src = input.begin();

    while (src != input.end() && *src == 0xfe00) {
        src++;  // Skip padding
    }

    if (src == input.end()) return false;
    DCT dct = *src++;
    int32_t current = extend_sign<10>(dct.dc);

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

        if (src == input.end()) return false;
        RLE rle = *src++;
        current = extend_sign<10>(rle.ac);
        n += rle.zeroes + 1;

        value = (current * table[n] * dct.qFactor + 4) / 8;
    }

    idct(blk);
    return true;
}

}  // namespace mdec