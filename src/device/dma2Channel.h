#pragma once
#include "dmaChannel.h"

namespace device {
namespace dma {
namespace dmaChannel {
class DMA2Channel : public DMAChannel {
    device::gpu::GPU *gpu = nullptr;

    uint32_t readDevice() { 
        return gpu->read(0) | (gpu->read(1) << 8) | (gpu->read(2) << 16) | (gpu->read(3) << 24);
	}
    void writeDevice(uint32_t data) {
        gpu->write(0, data);
        gpu->write(1, data >> 8);
        gpu->write(2, data >> 16);
        gpu->write(3, data >> 24);
    }

   public:
    DMA2Channel(int channel) : DMAChannel(channel) {}

    void setGPU(device::gpu::GPU *gpu) { this->gpu = gpu; }
};
}
}
}