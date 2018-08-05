#pragma once
#include "device/device.h"

namespace spu {
    union Volume {
        struct {
            uint16_t left;
            uint16_t right;
        };
        uint32_t _reg;
        uint8_t _byte[4];

        Volume() : left(0), right(0) {}

        void write(int n, uint8_t v) {
            if (n >= 4) return;
            _byte[n] = v;
        }

        uint8_t read(int n) const {
            if (n >= 4) return 0;
            return _byte[n];
        }

        float getLeft() {
            if (left & 0x8000) return 1.f;  // TODO: Implement ADSR
            return ((int16_t)(left << 1) / (float)0x7fff);
        }

        float getRight() {
            if (right & 0x8000) return 1.f;  // TODO: Implement ADSR
            return ((int16_t)(right << 1) / (float)0x7fff);
        }
    };

    union DataTransferControl {
        struct {
            uint16_t : 1;
            uint16_t transferType : 3;
            uint16_t : 12;
        };
        uint16_t _reg;
        uint8_t _byte[2];

        DataTransferControl() : _reg(0) {}
    };

    union Control {
        struct {
            uint16_t cdEnable : 1;
            uint16_t externalEnable : 1;
            uint16_t cdReverb : 1;
            uint16_t externalReverb : 1;
            uint16_t transferType : 2;  // 0 - stop, 1 - Manual, 2 - DMA Write, 3 - DMA Read
            uint16_t irqEnable : 1;
            uint16_t masterReverb : 1;
            uint16_t noiseFrequencyStep : 2;  // 4,5,6,7
            uint16_t noiseFrequencyShift : 4;
            uint16_t mute : 1;  // 0 - mute, 1 - unmute
            uint16_t spuEnable : 1;
        };
        uint16_t _reg;
        uint8_t _byte[2];

        Control() : _reg(0) {}
    };

}