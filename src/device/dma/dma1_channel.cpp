#include "dma1_channel.h"
#include "device/mdec/mdec.h"

namespace device::dma {
DMA1Channel::DMA1Channel(Channel channel, System* sys, mdec::MDEC* mdec) : DMAChannel(channel, sys), mdec(mdec) {}

uint32_t DMA1Channel::readDevice() { return mdec->read<true>(0); }

bool DMA1Channel::dataRequest() { return mdec->dataOutRequest(); }
}  // namespace device::dma