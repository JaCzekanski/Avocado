#pragma once
#include "dma_channel.h"

namespace device::dma::dmaChannel {
class DMA6Channel : public DMAChannel {
    uint32_t readDevice() override;

   public:
    DMA6Channel(int channel, System* sys);
};
}  // namespace device::dma::dmaChannel