#include "mdec.h"

namespace mdec {
void MDEC::idct(std::array<int16_t, 64>& src) {
    std::array<int64_t, 64> tmp = {{}};

    for (int x = 0; x < 8; x++) {
        for (int y = 0; y < 8; y++) {
            int64_t sum = 0;
            for (int i = 0; i < 8; i++) {
                sum += idctTable[i * 8 + y] * src[x + i * 8];
            }
            tmp[x + y * 8] = sum;
        }
    }

    for (int x = 0; x < 8; x++) {
        for (int y = 0; y < 8; y++) {
            int64_t sum = 0;
            for (int i = 0; i < 8; i++) {
                sum += tmp[i + y * 8] * idctTable[x + i * 8];
            }

            int round = (sum >> 31) & 1;
            src[x + y * 8] = (uint16_t)((sum >> 32) + round);
        }
    }
}
}  // namespace mdec