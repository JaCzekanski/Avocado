#pragma once
#include "dma_channel.h"

class MDEC;

namespace device::dma::dmaChannel {
class DMA0Channel : public DMAChannel {
    MDEC* mdec;

    uint32_t readDevice() override;
    void writeDevice(uint32_t data) override;

   public:
    DMA0Channel(int channel, System* sys, MDEC* mdec);
};
}  // namespace device::dma::dmaChannel