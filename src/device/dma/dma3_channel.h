#pragma once
#include "dma_channel.h"

namespace device::cdrom {
class CDROM;
}

namespace device::dma {
class DMA3Channel : public DMAChannel {
    device::cdrom::CDROM* cdrom;

    uint32_t readDevice() override;

   public:
    DMA3Channel(Channel channel, System* sys, device::cdrom::CDROM* cdrom);
};
}  // namespace device::dma