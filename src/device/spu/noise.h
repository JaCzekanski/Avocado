#pragma once
#include <cstdint>

namespace spu {
class Noise {
    int frequency = 0;
    int16_t level = 0;

   public:
    int16_t getNoiseLevel();
    void doNoise(uint16_t step, uint16_t shift);

    template <class Archive>
    void serialize(Archive& ar) {
        ar(frequency, level);
    }
};
}  // namespace spu