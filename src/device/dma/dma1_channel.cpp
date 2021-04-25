#include "dma1_channel.h"
#include "device/mdec/mdec.h"

namespace device::dma {
DMA1Channel::DMA1Channel(Channel channel, System* sys, mdec::MDEC* mdec) : DMAChannel(channel, sys), mdec(mdec) {}

uint32_t DMA1Channel::readDevice() { return mdec->read(0); }

bool DMA1Channel::dataRequest() { return mdec->dataOutRequest(); }

bool DMA1Channel::hack_supportChoppedTransfer() const { return true; }
}  // namespace device::dma