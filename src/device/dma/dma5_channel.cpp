#include "dma5_channel.h"

namespace device::dma {
DMA5Channel::DMA5Channel(Channel channel, System* sys) : DMAChannel(channel, sys) {}
}  // namespace device::dma