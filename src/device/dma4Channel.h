#pragma once
#include "dmaChannel.h"
#include "spu.h"

namespace device {
namespace dma {
namespace dmaChannel {
class DMA4Channel : public DMAChannel {
    SPU *spu = nullptr;

    uint32_t readDevice() override { return 0; }
    void writeDevice(uint32_t data) override {
        spu->write(0x1a8, data);
        spu->write(0x1a8, data >> 8);
        spu->write(0x1a8, data >> 16);
        spu->write(0x1a8, data >> 24);
    }

   public:
    DMA4Channel(int channel, System *sys, SPU *spu) : DMAChannel(channel, sys), spu(spu) {}
};
}  // namespace dmaChannel
}  // namespace dma
}  // namespace device
