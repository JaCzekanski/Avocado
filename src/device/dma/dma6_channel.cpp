#include "dma6_channel.h"

namespace device::dma {
uint32_t DMA6Channel::readDevice() { return 0xffffffff; }

DMA6Channel::DMA6Channel(Channel channel, System* sys) : DMAChannel(channel, sys) {}
}  // namespace device::dma