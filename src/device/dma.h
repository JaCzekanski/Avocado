#pragma once
#include "device.h"
#include "dmaChannel.h"
#include "dma2Channel.h"
#include "dma6Channel.h"
#include "gpu.h"
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

class DMA : public Device {
    DPCR control;
    uint32_t dmaStatus;
    // uint32_t dma2Control;
    // MADDR dma2Address;
    // uint32_t dma2Count = 0;
    // CHCR dma6Control;
    // MADDR dma6Address;
    // BCR dma6Count;

    dmaChannel::DMA2Channel dma2;

   public:
    dmaChannel::DMA3Channel dma3;

   private:
    dmaChannel::DMA6Channel dma6;

    void *_cpu = nullptr;
    gpu::GPU *gpu = nullptr;

    void writeDma2(uint32_t address, uint8_t data);
    void writeDma6(uint32_t address, uint8_t data);

   public:
    DMA();
    void step();
    uint8_t read(uint32_t address);
    void write(uint32_t address, uint8_t data);

    void setCPU(void *cpu) {
        this->_cpu = cpu;
        dma2.setCPU(_cpu);
        dma3.setCPU(_cpu);
        dma6.setCPU(_cpu);
    }

    void setGPU(gpu::GPU *gpu) {
        this->gpu = gpu;
        dma2.setGPU(gpu);
    }
};
}
}
