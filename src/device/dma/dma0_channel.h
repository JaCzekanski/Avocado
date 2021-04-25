#pragma once
#include "dma_channel.h"

namespace mdec {
class MDEC;
};

namespace device::dma {
class DMA0Channel : public DMAChannel {
    mdec::MDEC* mdec;

    void writeDevice(uint32_t data) override;
    bool dataRequest() override;

   public:
    DMA0Channel(Channel channel, System* sys, mdec::MDEC* mdec);
};
}  // namespace device::dma