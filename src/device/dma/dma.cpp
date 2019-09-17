#include "dma.h"
#include <fmt/core.h>
#include "dma0_channel.h"
#include "dma1_channel.h"
#include "dma2_channel.h"
#include "dma3_channel.h"
#include "dma4_channel.h"
#include "dma5_channel.h"
#include "dma6_channel.h"
#include "system.h"

namespace device::dma {
DMA::DMA(System* sys) : sys(sys) {
    dma[0] = std::make_unique<DMA0Channel>(Channel::MDECin, sys, sys->mdec.get());
    dma[1] = std::make_unique<DMA1Channel>(Channel::MDECout, sys, sys->mdec.get());
    dma[2] = std::make_unique<DMA2Channel>(Channel::GPU, sys, sys->gpu.get());
    dma[3] = std::make_unique<DMA3Channel>(Channel::CDROM, sys, sys->cdrom.get());
    dma[4] = std::make_unique<DMA4Channel>(Channel::SPU, sys, sys->spu.get());
    dma[5] = std::make_unique<DMA5Channel>(Channel::PIO, sys);
    dma[6] = std::make_unique<DMA6Channel>(Channel::OTC, sys);
}

void DMA::step() {
    if (pendingInterrupt) {
        pendingInterrupt = false;
        sys->interrupt->trigger(interrupt::DMA);
    }
}

uint8_t DMA::read(uint32_t address) {
    int channel = address / 0x10;
    if (channel < 7) return dma[channel]->read(address % 0x10);

    // control
    address += 0x80;
    if (address >= 0xf0 && address < 0xf4) {
        return control._byte[address - 0xf0];
    }
    if (address >= 0xf4 && address < 0xf8) {
        return status._byte[address - 0xf4];
    }
    return 0;
}

void DMA::write(uint32_t address, uint8_t data) {
    int channel = address / 0x10;
    if (channel > 6)  // control
    {
        address += 0x80;
        if (address >= 0xF0 && address < 0xf4) {
            control._byte[address - 0xf0] = data;
            return;
        }
        if (address >= 0xF4 && address < 0xf8) {
            status.write(address - 0xf4, data);
            return;
        }
        fmt::print("W Unimplemented DMA address 0x{:08x}\n", address);
        return;
    }

    dma[channel]->write(address % 0x10, data);
    if (dma[channel]->irqFlag) {
        dma[channel]->irqFlag = false;
        if (status.getEnableDma(channel)) {
            status.setFlagDma(channel, 1);
            pendingInterrupt = status.calcMasterFlag();
        }
    }
}
}  // namespace device::dma