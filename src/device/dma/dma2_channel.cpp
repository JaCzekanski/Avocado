#include "dma2_channel.h"
#include "device/gpu/gpu.h"

namespace device::dma {
DMA2Channel::DMA2Channel(Channel channel, System *sys, gpu::GPU *gpu) : DMAChannel(channel, sys), gpu(gpu) {}

uint32_t DMA2Channel::readDevice() { return gpu->read(0); }

void DMA2Channel::writeDevice(uint32_t data) { gpu->write(0, data); }

bool DMA2Channel::dataRequest() { return gpu->dmaDataRequest(); }
}  // namespace device::dma