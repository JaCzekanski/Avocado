#pragma once
#include "dmaChannel.h"

namespace device {
namespace dma {
namespace dmaChannel {
class DMA2Channel : public DMAChannel {
    gpu::GPU *gpu = nullptr;

    uint32_t readDevice() override { return gpu->read(0); }
    void writeDevice(uint32_t data) override { gpu->write(0, data); }

   public:
    DMA2Channel(int channel) : DMAChannel(channel) {}

    void setGPU(gpu::GPU *gpu) { this->gpu = gpu; }
};
}
}
}