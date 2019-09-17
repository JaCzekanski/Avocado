#pragma once
#include "dma_channel.h"

namespace spu {
struct SPU;
}

namespace device::dma {
class DMA4Channel : public DMAChannel {
    spu::SPU *spu = nullptr;

    void writeDevice(uint32_t data) override;

   public:
    DMA4Channel(Channel channel, System *sys, spu::SPU *spu);
};
}  // namespace device::dma