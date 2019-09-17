#pragma once
#include "dma_channel.h"

namespace device::dma {
class DMA6Channel : public DMAChannel {
    uint32_t readDevice() override;

   public:
    DMA6Channel(Channel channel, System* sys);
};
}  // namespace device::dma