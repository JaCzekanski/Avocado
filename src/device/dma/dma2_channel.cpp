#include "dma2_channel.h"
#include "device/gpu/gpu.h"

namespace device::dma::dmaChannel {
DMA2Channel::DMA2Channel(int channel, System *sys, gpu::GPU *gpu) : DMAChannel(channel, sys), gpu(gpu) {}

uint32_t DMA2Channel::readDevice() { return gpu->read(0); }

void DMA2Channel::writeDevice(uint32_t data) { gpu->write(0, data); }
}  // namespace device::dma::dmaChannel