#pragma once
#include "dma_channel.h"

namespace gpu {
class GPU;
}

namespace device::dma {
class DMA2Channel : public DMAChannel {
    gpu::GPU *gpu;

    uint32_t readDevice() override;
    void writeDevice(uint32_t data) override;

   protected:
    bool dataRequest() override;

   public:
    DMA2Channel(Channel channel, System *sys, gpu::GPU *gpu);
};
}  // namespace device::dma