#include "noise.h"
#include <array>

namespace {
std::array<int8_t, 64> noiseAddition = {{
    1, 0, 0, 1, 0, 1, 1, 0,  //
    1, 0, 0, 1, 0, 1, 1, 0,  //
    1, 0, 0, 1, 0, 1, 1, 0,  //
    1, 0, 0, 1, 0, 1, 1, 0,  //
    0, 1, 1, 0, 1, 0, 0, 1,  //
    0, 1, 1, 0, 1, 0, 0, 1,  //
    0, 1, 1, 0, 1, 0, 0, 1,  //
    0, 1, 1, 0, 1, 0, 0, 1   //
}};

std::array<uint8_t, 5> noiseHalfcycle = {{0, 84, 140, 180, 210}};
};  // namespace

namespace spu {
int16_t Noise::getNoiseLevel() { return level; }

void Noise::doNoise(uint16_t step, uint16_t shift) {
    int freq = (0x8000 >> shift) << 16;

    frequency += 0x10000;
    frequency += noiseHalfcycle[step];

    if ((frequency & 0xffff) >= noiseHalfcycle[4]) {
        frequency += 0x10000;
        frequency -= noiseHalfcycle[step];
    }

    if (frequency >= freq) {
        frequency %= freq;

        int index = (level >> 10) & 0x3f;
        level = (level << 1) + noiseAddition[index];
    }
}
}  // namespace spu