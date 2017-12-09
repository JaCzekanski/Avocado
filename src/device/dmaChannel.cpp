#include "dmaChannel.h"
#include "mips.h"
#include <cstdio>

namespace device {
namespace dma {
namespace dmaChannel {
DMAChannel::DMAChannel(int channel, mips::CPU *cpu) : channel(channel), cpu(cpu) {}

void DMAChannel::step() {}

uint8_t DMAChannel::read(uint32_t address) {
    if (address < 0x4) return baseAddress._byte[address];
    if (address >= 0x4 && address < 0x8) return count._byte[address - 4];
    if (address >= 0x8 && address < 0xc) return control._byte[address - 8];

    printf("R Unimplemented DMA%d address 0x%08x\n", channel, address);
    return 0;
}

void DMAChannel::write(uint32_t address, uint8_t data) {
    if (address < 0x4)
        baseAddress._byte[address] = data;
    else if (address >= 0x4 && address < 0x8)
        count._byte[address - 4] = data;
    else if (address >= 0x8 && address < 0xc) {
        control._byte[address - 0x8] = data;

        if (control.enabled != CHCR::Enabled::start) return;
        control.startTrigger = CHCR::StartTrigger::clear;

        if (control.syncMode == CHCR::SyncMode::startImmediately) {
            //            printf("DMA%d mode: word @ 0x%08x\n", channel, baseAddress.address);

            // TODO: Check Transfer Direction
            // TODO: Check Memory Address Step

            int addr = baseAddress.address;
            if (channel == 3)  // CDROM
            {
                beforeRead();
                if (verbose) printf("DMA%d CDROM -> CPU @ 0x%08x, count: 0x%04x\n", channel, addr, count.syncMode0.wordCount);
                for (size_t i = 0; i < count.syncMode0.wordCount; i++) {
                    cpu->writeMemory32(addr, readDevice());
                    addr += 4;
                }
                // cpu->cdrom->status.dataFifoEmpty = 0;
            } else {
                for (size_t i = 0; i < count.syncMode0.wordCount; i++) {
                    if (i == count.syncMode0.wordCount - 1)
                        cpu->writeMemory32(addr, 0xffffff);
                    else
                        cpu->writeMemory32(addr, (addr - 4) & 0xffffff);
                    addr -= 4;
                }
            }
            control.enabled = CHCR::Enabled::stop;
        } else if (control.syncMode == CHCR::SyncMode::syncBlockToDmaRequests) {
            uint32_t addr = baseAddress.address;
            int blockCount = count.syncMode1.blockCount;
            int blockSize = count.syncMode1.blockSize;
            if (blockCount == 0) blockCount = 0x10000;

            if (control.transferDirection == CHCR::TransferDirection::toMainRam)  // VRAM READ
            {
                //               printf("DMA%d VRAM -> CPU @ 0x%08x, BS: 0x%04x, BC: 0x%04x\n", channel, addr, blockSize, blockCount);

                for (int block = 0; block < blockCount; block++) {
                    for (int i = 0; i < blockSize; i++, addr += 4) {
                        cpu->writeMemory32(addr, readDevice());
                    }
                }
            } else if (control.transferDirection == CHCR::TransferDirection::fromMainRam)  // VRAM WRITE
            {
                if (channel == 3 && verbose) {
                    printf("DMA%d CPU -> VRAM @ 0x%08x, BS: 0x%04x, BC: 0x%04x\n", channel, addr, blockSize, blockCount);
                }
                if (channel == 4 && verbose) {
                    printf("DMA%d CPU -> SPU @ 0x%08x, BS: 0x%04x, BC: 0x%04x\n", channel, addr, blockSize, blockCount);
                }

                for (int block = 0; block < blockCount; block++) {
                    for (int i = 0; i < blockSize; i++, addr += 4) {
                        writeDevice(cpu->readMemory32(addr));
                    }
                }
            }
        } else if (control.syncMode == CHCR::SyncMode::linkedListMode) {
            //           printf("DMA%d linked list\n", channel);
            int addr = baseAddress.address;

            int breaker = 0;
            for (;;) {
                uint32_t blockInfo = cpu->readMemory32(addr);
                int commandCount = blockInfo >> 24;

                addr += 4;
                for (int i = 0; i < commandCount; i++, addr += 4) {
                    writeDevice(cpu->readMemory32(addr));
                }
                addr = blockInfo & 0xffffff;
                if (addr == 0xffffff || addr == 0) break;

                if (++breaker > 0x4000) {
                    printf("GPU DMA transfer too long, breaking.\n");
                    break;
                }
            }
        }

        irqFlag = true;
        control.enabled = CHCR::Enabled::completed;
    }
}
}
}
}
