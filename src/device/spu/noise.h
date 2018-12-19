#pragma once
#include <cstdint>

namespace spu {
struct Noise {
    int noiseFrequency;
    int16_t noiseLevel;

    Noise();
    int16_t getNoiseLevel();

    void doNoise(uint16_t step, uint16_t shift);
};
}  // namespace spu