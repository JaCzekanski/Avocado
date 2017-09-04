#include "dma.h"
#include "mips.h"
#include <cstdio>

namespace device {
namespace dma {
DMA::DMA() : dma0(0), dma1(1), dma2(2), dma3(3), dma4(4), dma5(5), dma6(6) {}

void DMA::step() {
    bool prevMasterFlag = status.masterFlag;

    uint8_t enables = (status._reg & 0x7F0000) >> 16;
    uint8_t flags = (status._reg & 0x7F000000) >> 24;
    status.masterFlag = status.forceIRQ || (status.masterEnable && (enables & flags));

    if (!prevMasterFlag && status.masterFlag) {
        ((mips::CPU*)_cpu)->interrupt->trigger(interrupt::DMA);
    }
}

uint8_t DMA::read(uint32_t address) {
    int channel = address / 0x10;
    if (channel == 0) return dma0.read(address % 0x10);  // MDECin
    if (channel == 1) return dma1.read(address % 0x10);  // MDECout
    if (channel == 2) return dma2.read(address % 0x10);  // GPU
    if (channel == 3) return dma3.read(address % 0x10);  // CDROM
    if (channel == 4) return dma4.read(address % 0x10);  // SPU
    if (channel == 5) return dma5.read(address % 0x10);  // PIO
    if (channel == 6) return dma6.read(address % 0x10);  // reverse clear OT
    if (channel > 6)                                     // control
    {
        address += 0x80;
        if (address >= 0xf0 && address < 0xf4) {
            return control._byte[address - 0xf0];
        }
        if (address >= 0xf4 && address < 0xf8) {
            return status._byte[address - 0xf4];
        }
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

    if (channel == 0) {
        dma0.write(address % 0x10, data);  // MDECin

        if (dma0.irqFlag) {
            dma0.irqFlag = false;
            if (status.enableDma0) status.flagDma0 = 1;
        }
        return;
    }
    if (channel == 1) {
        dma1.write(address % 0x10, data);  // MDECout

        if (dma1.irqFlag) {
            dma1.irqFlag = false;
            if (status.enableDma1) status.flagDma1 = 1;
        }
        return;
    }
    if (channel == 2) {
        dma2.write(address % 0x10, data);  // GPU

        if (dma2.irqFlag) {
            dma2.irqFlag = false;
            if (status.enableDma2) status.flagDma2 = 1;
        }
        return;
    }
    if (channel == 3) {
        dma3.write(address % 0x10, data);  // CDROM

        if (dma3.irqFlag) {
            dma3.irqFlag = false;
            if (status.enableDma3) status.flagDma3 = 1;
        }
        return;
    }
    if (channel == 4) {
        dma4.write(address % 0x10, data);

        if (dma4.irqFlag) {
            dma4.irqFlag = false;
            if (status.enableDma4) status.flagDma4 = 1;
        }
        return;
    }
    if (channel == 5) {
        dma5.write(address % 0x10, data);

        if (dma5.irqFlag) {
            dma5.irqFlag = false;
            if (status.enableDma5) status.flagDma5 = 1;
        }
        return;
    }
    if (channel == 6) {
        dma6.write(address % 0x10, data);  // reverse clear OT

        if (dma6.irqFlag) {
            dma6.irqFlag = false;
            if (status.enableDma6) status.flagDma6 = 1;
        }
    }
}
}
}
