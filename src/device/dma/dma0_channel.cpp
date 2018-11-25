#include "dma0_channel.h"
#include "device/mdec.h"

namespace device::dma::dmaChannel {
DMA0Channel::DMA0Channel(int channel, System* sys, MDEC* mdec) : DMAChannel(channel, sys), mdec(mdec) {}

uint32_t DMA0Channel::readDevice() { return 0; }

void DMA0Channel::writeDevice(uint32_t data) { mdec->write(0, data); }
}  // namespace device::dma::dmaChannel