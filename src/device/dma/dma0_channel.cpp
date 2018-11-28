#include "dma0_channel.h"
#include "device/mdec/mdec.h"

namespace device::dma::dmaChannel {
DMA0Channel::DMA0Channel(int channel, System* sys, mdec::MDEC* mdec) : DMAChannel(channel, sys), mdec(mdec) {}

void DMA0Channel::writeDevice(uint32_t data) { mdec->write(0, data); }
}  // namespace device::dma::dmaChannel