#pragma once
#include <array>
#include "dma_channel.h"
#include "utils/file.h"

namespace device::cdrom {
class CDROM;
}

namespace device::dma::dmaChannel {
class DMA3Channel : public DMAChannel {
    device::cdrom::CDROM* cdrom;
    uint32_t readDevice() override;

   public:
    DMA3Channel(int channel, System* sys, device::cdrom::CDROM* cdrom);
};
}  // namespace device::dma::dmaChannel