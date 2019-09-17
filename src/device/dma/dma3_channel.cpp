#include "dma3_channel.h"
#include "device/cdrom/cdrom.h"
#include "utils/file.h"

namespace device::dma {
uint32_t DMA3Channel::readDevice() {
    uint32_t data = 0;
    data |= cdrom->readByte() << 0;
    data |= cdrom->readByte() << 8;
    data |= cdrom->readByte() << 16;
    data |= cdrom->readByte() << 24;
    return data;
}

DMA3Channel::DMA3Channel(Channel channel, System* sys, device::cdrom::CDROM* cdrom) : DMAChannel(channel, sys), cdrom(cdrom) {}

}  // namespace device::dma
