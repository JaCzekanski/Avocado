#pragma once
#include "device.h"
#include "dmaChannel.h"
#include "dma2Channel.h"
#include "dma6Channel.h"
#include "gpu/gpu.h"
#include "dma3Channel.h"

namespace device {
namespace dma {

// 0x1f8010f0 - DPCR,  DMA Control
union DPCR {
    struct {
        uint32_t priorityMdecIn : 3;  // DMA0
        Bit enableMdecIn : 1;         // DMA0

        uint32_t priorityMdecOut : 3;  // DMA1
        Bit enableMdecOut : 1;         // DMA1

        uint32_t priorityGpu : 3;  // DMA2
        Bit enableGpu : 1;         // DMA2

        uint32_t priorityCdrom : 3;  // DMA3
        Bit enableCdrom : 1;         // DMA3

        uint32_t prioritySpu : 3;  // DMA4
        Bit enableSpu : 1;         // DMA4

        uint32_t priorityPio : 3;  // DMA5
        Bit enablePio : 1;         // DMA5

        uint32_t priorityOtc : 3;  // DMA6
        Bit enableOtc : 1;         // DMA6

        uint32_t : 4;
    };
    uint32_t _reg;
    uint8_t _byte[4];

    DPCR() : _reg(0) {}
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
};

class DMA {
    DPCR control;
    DICR status;

    dmaChannel::DMAChannel dma0;  // MDECin
    dmaChannel::DMAChannel dma1;  // MDECout
    dmaChannel::DMA2Channel dma2;

   public:
    dmaChannel::DMA3Channel dma3;

   private:
    dmaChannel::DMAChannel dma4;  // SPU
    dmaChannel::DMAChannel dma5;  // PIO?
    dmaChannel::DMA6Channel dma6;

   private:
    mips::CPU *cpu;
    GPU *gpu = nullptr;

    void writeDma2(uint32_t address, uint8_t data);
    void writeDma6(uint32_t address, uint8_t data);

   public:
    DMA(mips::CPU *cpu);
    void step();
    uint8_t read(uint32_t address);
    void write(uint32_t address, uint8_t data);
};
}
}
