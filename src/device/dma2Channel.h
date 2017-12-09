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
    DMA2Channel(int channel, mips::CPU *cpu, device::gpu::GPU *gpu) : DMAChannel(channel, cpu), gpu(gpu) {}
};
}
}
}