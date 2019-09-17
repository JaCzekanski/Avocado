#include "dma4_channel.h"
#include "device/spu/spu.h"

namespace device::dma {
DMA4Channel::DMA4Channel(Channel channel, System *sys, spu::SPU *spu) : DMAChannel(channel, sys), spu(spu) {}

void DMA4Channel::writeDevice(uint32_t data) {
    spu->write(0x1a8, data);
    spu->write(0x1a8, data >> 8);
    spu->write(0x1a8, data >> 16);
    spu->write(0x1a8, data >> 24);
}
}  // namespace device::dma