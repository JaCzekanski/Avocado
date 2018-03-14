#include "adpcm.h"
#include <cassert>

namespace ADPCM {
int filterTablePos[5] = {0, 60, 115, 98, 122};
int filterTableNeg[5] = {0, 0, -52, -55, -60};

std::vector<int16_t> decode(uint8_t* buffer, size_t sampleCount) {
    std::vector<int16_t> decoded;

    int16_t prevSample[2] = {0};

    for (size_t i = 0; i < sampleCount; i++) {
        uint8_t shift = buffer[i * 16 + 0] & 0x0f;
        uint8_t filter = (buffer[i * 16 + 0] & 0x70) >> 4;  // 0x40 for xa adpcm

        assert(filter <= 4);

        /*
         * 7654 3210
         *       ^^^-- Loop End
         *       ||--- Loop Repeat
         *       |---- Loop Start
         */
        uint8_t flag = buffer[i * 16 + 1];

        int filterPos = filterTablePos[filter];
        int filterNeg = filterTableNeg[filter];

        /* Sample byte
         *
         * MSB   LSB
         * YYYY XXXX
         *      1st sample
         *
         * 2nd sample
         */

        for (int n = 0; n < 28; n++) {
            int16_t sample;
            if (n % 2 == 0)
                sample = (buffer[i * 16 + 2 + n / 2] & 0x0f);
            else
                sample = (buffer[i * 16 + 2 + n / 2] & 0xf0) >> 4;

            // Extend 4bit sample to 16bit
            sample <<= 12;

            // Shift right by value in header
            sample >>= shift;

            sample += (prevSample[0] * filterPos + prevSample[1] * filterNeg + 32) / 64;

            // clamp to -0x8000 +0x7fff

            decoded.push_back(sample);

            prevSample[1] = prevSample[0];
            prevSample[0] = sample;
        }
    }

    return decoded;
}
}  // namespace ADPCM
