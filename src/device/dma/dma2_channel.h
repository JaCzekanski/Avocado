#pragma once
#include "dma_channel.h"

namespace gpu {
class GPU;
}

namespace device::dma::dmaChannel {
class DMA2Channel : public DMAChannel {
    gpu::GPU *gpu = nullptr;

    uint32_t readDevice() override;
    void writeDevice(uint32_t data) override;

   public:
    DMA2Channel(int channel, System *sys, gpu::GPU *gpu);
};
}  // namespace device::dma::dmaChannel