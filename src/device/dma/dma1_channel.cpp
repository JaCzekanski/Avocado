#include "dma1_channel.h"
#include "device/mdec.h"

namespace device::dma::dmaChannel {
DMA1Channel::DMA1Channel(int channel, System* sys, MDEC* mdec) : DMAChannel(channel, sys), mdec(mdec) {}

uint32_t DMA1Channel::readDevice() { return mdec->read(0); }

void DMA1Channel::writeDevice(uint32_t data) {}
}  // namespace device::dma::dmaChannel