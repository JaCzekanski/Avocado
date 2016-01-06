#pragma once
#include "device.h"
#include "dmaChannel.h"
#include "dma2Channel.h"
#include "dma6Channel.h"
#include "gpu.h"

namespace device {
namespace dma {
class DMA : public Device {
    uint32_t dmaControl;
    uint32_t dmaStatus;
    // uint32_t dma2Control;
    // MADDR dma2Address;
    // uint32_t dma2Count = 0;
    // CHCR dma6Control;
    // MADDR dma6Address;
    // BCR dma6Count;

    dmaChannel::DMA2Channel dma2;
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
        dma6.setCPU(_cpu);
    }

    void setGPU(gpu::GPU *gpu) {
        this->gpu = gpu;
        dma2.setGPU(gpu);
    }
};
}
}