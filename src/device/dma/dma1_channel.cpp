#include "dma1_channel.h"
#include "device/mdec/mdec.h"

namespace device::dma::dmaChannel {
DMA1Channel::DMA1Channel(int channel, System* sys, mdec::MDEC* mdec) : DMAChannel(channel, sys), mdec(mdec) {}

uint32_t DMA1Channel::readDevice() { return mdec->read(0); }
}  // namespace device::dma::dmaChannel