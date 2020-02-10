#include "dma4_channel.h"
#include "device/spu/spu.h"

namespace device::dma {
DMA4Channel::DMA4Channel(Channel channel, System *sys, spu::SPU *spu) : DMAChannel(channel, sys), spu(spu) {}

uint32_t DMA4Channel::readDevice() {
    // TODO: Do direct RAM write/read using memoryRead
    uint32_t data = 0;
    data |= spu->read(0x1a8);
    data |= spu->read(0x1a8) << 8;
    data |= spu->read(0x1a8) << 16;
    data |= spu->read(0x1a8) << 24;
    return data;
}
void DMA4Channel::writeDevice(uint32_t data) {
    spu->write(0x1a8, data);
    spu->write(0x1a8, data >> 8);
    spu->write(0x1a8, data >> 16);
    spu->write(0x1a8, data >> 24);
}
}  // namespace device::dma