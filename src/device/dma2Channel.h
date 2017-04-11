#pragma once
#include "dmaChannel.h"

namespace device {
namespace dma {
namespace dmaChannel {
class DMA2Channel : public DMAChannel {
    device::gpu::GPU *gpu = nullptr;

    uint32_t readDevice() { return gpu->read(0); }
    void writeDevice(uint32_t data) { gpu->write(0, data); }

   public:
    DMA2Channel(int channel) : DMAChannel(channel) {}

    void setGPU(device::gpu::GPU *gpu) { this->gpu = gpu; }
};
}
}
}