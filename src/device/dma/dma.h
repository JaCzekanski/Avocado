#pragma once
#include <memory>
#include "dma_channel.h"

struct System;

namespace device::dma {
// 0x1f8010f0 - DPCR,  DMA Control
union DPCR {
    struct {
        uint32_t priorityMdecIn : 3;  // DMA0
        uint32_t enableMdecIn : 1;    // DMA0

        uint32_t priorityMdecOut : 3;  // DMA1
        uint32_t enableMdecOut : 1;    // DMA1

        uint32_t priorityGpu : 3;  // DMA2
        uint32_t enableGpu : 1;    // DMA2

        uint32_t priorityCdrom : 3;  // DMA3
        uint32_t enableCdrom : 1;    // DMA3

        uint32_t prioritySpu : 3;  // DMA4
        uint32_t enableSpu : 1;    // DMA4

        uint32_t priorityPio : 3;  // DMA5
        uint32_t enablePio : 1;    // DMA5

        uint32_t priorityOtc : 3;  // DMA6
        uint32_t enableOtc : 1;    // DMA6

        uint32_t : 4;
    };
    uint32_t _reg;
    uint8_t _byte[4];

    DPCR() : _reg(0x07654321) {}
};

// 0x1f8010f4 - DICR,  DMA Interrupt Register
union DICR {
    struct {
        uint32_t : 15;

        uint32_t forceIRQ : 1;

        uint32_t enableDma0 : 1;
        uint32_t enableDma1 : 1;
        uint32_t enableDma2 : 1;
        uint32_t enableDma3 : 1;
        uint32_t enableDma4 : 1;
        uint32_t enableDma5 : 1;
        uint32_t enableDma6 : 1;
        uint32_t masterEnable : 1;

        uint32_t flagDma0 : 1;
        uint32_t flagDma1 : 1;
        uint32_t flagDma2 : 1;
        uint32_t flagDma3 : 1;
        uint32_t flagDma4 : 1;
        uint32_t flagDma5 : 1;
        uint32_t flagDma6 : 1;
        uint32_t masterFlag : 1;
    };
    uint32_t _reg;
    uint8_t _byte[4];

    DICR() : _reg(0) {}

    void write(int n, uint8_t value) {
        if (n <= 2) {
            _byte[n] = value;
        } else if (n == 3) {
            _byte[3] &= ~value;
        }
        masterFlag = calcMasterFlag();
    }

    bool calcMasterFlag() {
        uint8_t enables = (_reg & 0x7F0000) >> 16;
        uint8_t flags = (_reg & 0x7F000000) >> 24;
        return forceIRQ || (masterEnable && (enables & flags));
    }

    bool getEnableDma(int channel) { return _reg & (1 << (16 + channel)); }

    void setFlagDma(int channel, bool value) {
        if (value) {
            _reg |= (1 << (24 + channel));
        } else {
            _reg &= ~(1 << (24 + channel));
        }
    }
};

class DMA {
    std::unique_ptr<DMAChannel> dma[7];

    DPCR control;
    DICR status;
    bool pendingInterrupt = false;

    System* sys;

   public:
    DMA(System* sys);
    void reset();
    void step();
    uint8_t read(uint32_t address);
    void write(uint32_t address, uint8_t data);

    bool isChannelEnabled(Channel ch);

    template <class Archive>
    void serialize(Archive& ar) {
        ar(control._reg, status._reg, pendingInterrupt);
        for (auto& ch : dma) ar(*ch);
    }
};
}  // namespace device::dma