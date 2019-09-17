#pragma once
#include "dma_channel.h"

namespace mdec {
class MDEC;
};

namespace device::dma {
class DMA1Channel : public DMAChannel {
    mdec::MDEC* mdec;

    uint32_t readDevice() override;

   public:
    DMA1Channel(Channel channel, System* sys, mdec::MDEC* mdec);
};
}  // namespace device::dma