#pragma once
#include "dma_channel.h"

namespace mdec {
class MDEC;
};

namespace device::dma::dmaChannel {
class DMA0Channel : public DMAChannel {
    mdec::MDEC* mdec;

    uint32_t readDevice() override;
    void writeDevice(uint32_t data) override;

   public:
    DMA0Channel(int channel, System* sys, mdec::MDEC* mdec);
};
}  // namespace device::dma::dmaChannel