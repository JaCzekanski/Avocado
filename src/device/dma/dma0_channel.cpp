#include "dma0_channel.h"
#include "device/mdec/mdec.h"

namespace device::dma {
DMA0Channel::DMA0Channel(Channel channel, System* sys, mdec::MDEC* mdec) : DMAChannel(channel, sys), mdec(mdec) {}

void DMA0Channel::writeDevice(uint32_t data) { mdec->write(0, data); }

bool DMA0Channel::dataRequest() { return mdec->dataInRequest(); }
}  // namespace device::dma