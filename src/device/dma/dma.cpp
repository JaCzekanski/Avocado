#include "dma.h"
#include <cstdio>
#include "dma2_channel.h"
#include "dma3_channel.h"
#include "dma4_channel.h"
#include "dma6_channel.h"
#include "system.h"

namespace device::dma {
DMA::DMA(System* sys) : sys(sys) {
    dma[0] = std::make_unique<dmaChannel::DMAChannel>(0, sys);
    dma[1] = std::make_unique<dmaChannel::DMAChannel>(1, sys);
    dma[2] = std::make_unique<dmaChannel::DMA2Channel>(2, sys, sys->gpu.get());
    dma[3] = std::make_unique<dmaChannel::DMA3Channel>(3, sys, sys->cdrom.get());
    dma[4] = std::make_unique<dmaChannel::DMA4Channel>(4, sys, sys->spu.get());
    dma[5] = std::make_unique<dmaChannel::DMAChannel>(5, sys);
    dma[6] = std::make_unique<dmaChannel::DMA6Channel>(6, sys);
}

void DMA::step() {
    bool prevMasterFlag = status.masterFlag;

    uint8_t enables = (status._reg & 0x7F0000) >> 16;
    uint8_t flags = (status._reg & 0x7F000000) >> 24;
    status.masterFlag = status.forceIRQ || (status.masterEnable && (enables & flags));

    if (!prevMasterFlag && status.masterFlag) {
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
            if (address == 0xf7) {
                // Clear flags (by writing 1 to bit) which sets it to 0
                // do not touch master flag
                status._byte[address - 0xF4] &= 0x80 | ((~data) & 0x7f);
                return;
            }
            status._byte[address - 0xf4] = data;
            return;
        }
        printf("W Unimplemented DMA address 0x%08x\n", address);
        return;
    }

    dma[channel]->write(address % 0x10, data);
    if (dma[channel]->irqFlag) {
        dma[channel]->irqFlag = false;
        if (status.getEnableDma(channel)) status.setFlagDma(channel, 1);
    }
}
}  // namespace device::dma