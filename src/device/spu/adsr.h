#pragma once
#include "device/device.h"

namespace spu {
struct Envelope {
    enum class Direction { Increase = 0, Decrease = 1 };
    enum class Mode { Linear = 0, Exponential = 1 };

    int level;
    int step;  // 0..3
    int shift;
    Direction direction;
    Mode mode;

    int getStep() const;
};

union ADSR {
    struct {
        uint32_t sustainLevel : 4;

        // No decay step, always -8
        uint32_t decayShift : 4;
        // No decay direction, always decrease
        // No decay mode, always Exponential

        uint32_t attackStep : 2;
        uint32_t attackShift : 5;
        // No attack direction, always increase
        uint32_t attackMode : 1;

        // No release step, always -8
        uint32_t releaseShift : 5;
        // No release direction, always decrease
        uint32_t releaseMode : 1;

        uint32_t sustainStep : 2;
        uint32_t sustainShift : 5;
        uint32_t : 1;
        uint32_t sustainDirection : 1;  // 0 - increase, 1 - decrease
        uint32_t sustainMode : 1;       // 0 - linear, 1 - exponential
    };
    uint32_t _reg;
    uint8_t _byte[4];

    ADSR();
    void write(int n, uint8_t v);
    uint8_t read(int n) const;
    Envelope attack();
    Envelope decay();
    Envelope sustain();
    Envelope release();
};
}  // namespace spu