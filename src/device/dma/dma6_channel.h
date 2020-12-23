#pragma once
#include "dma_channel.h"

namespace device::dma {
class DMA6Channel : public DMAChannel {
    void maskControl() override;
    void burstTransfer() override;

   public:
    DMA6Channel(Channel channel, System* sys);
};
}  // namespace device::dma