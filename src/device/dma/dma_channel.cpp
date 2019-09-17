#include "dma_channel.h"
#include <fmt/core.h>
#include <magic_enum.hpp>
#include "config.h"
#include "system.h"

namespace device::dma {
DMAChannel::DMAChannel(Channel channel, System* sys) : channel(channel), sys(sys) { verbose = config["debug"]["log"]["dma"]; }

DMAChannel::~DMAChannel() {}

uint32_t DMAChannel::readDevice() { return 0; }

void DMAChannel::writeDevice(uint32_t data) { (void)data; }

uint8_t DMAChannel::read(uint32_t address) {
    if (address < 0x4) return baseAddress._byte[address];
    if (address >= 0x4 && address < 0x8) return count._byte[address - 4];
    if (address >= 0x8 && address < 0xc) return control._byte[address - 8];

    fmt::print("R Unimplemented DMA%d address 0x%08x\n", (int)channel, address);
    return 0;
}

void DMAChannel::write(uint32_t address, uint8_t data) {
    if (address < 0x4)
        baseAddress._byte[address] = data;
    else if (address >= 0x4 && address < 0x8)
        count._byte[address - 4] = data;
    else if (address >= 0x8 && address < 0xc) {
        control._byte[address - 0x8] = data;

        if (address != 0xb) return;

        if (control.enabled != CHCR::Enabled::start) return;
        control.startTrigger = CHCR::StartTrigger::clear;

        if (control.syncMode == CHCR::SyncMode::startImmediately) {
            int addr = baseAddress.address;
            // TODO: Check Transfer Direction
            // TODO: Check Memory Address Step

            if (verbose) {
                if (control.direction == CHCR::Direction::toRam) {
                    fmt::print("[DMA{}] {:<8} -> RAM @ 0x{:08x}, block, count: 0x{:04x}\n", (int)channel, magic_enum::enum_name(channel),
                               addr, static_cast<int>(count.syncMode0.wordCount));
                } else if (control.direction == CHCR::Direction::fromRam) {
                    fmt::print("[DMA{}] {:<8} <- RAM @ 0x{:08x}, block, count: 0x{:04x}\n", (int)channel, magic_enum::enum_name(channel),
                               addr, static_cast<int>(count.syncMode0.wordCount));
                }
            }
            if (channel == Channel::CDROM) {
                for (size_t i = 0; i < count.syncMode0.wordCount; i++) {
                    sys->writeMemory32(addr, readDevice());
                    addr += 4;
                }
            } else {
                for (size_t i = 0; i < count.syncMode0.wordCount; i++) {
                    if (i == count.syncMode0.wordCount - 1) {
                        sys->writeMemory32(addr, 0xffffff);
                    } else {
                        sys->writeMemory32(addr, (addr - 4) & 0xffffff);
                    }
                    addr -= 4;
                }
            }
            control.enabled = CHCR::Enabled::stop;
        } else if (control.syncMode == CHCR::SyncMode::syncBlockToDmaRequests) {
            uint32_t addr = baseAddress.address;
            int blockCount = count.syncMode1.blockCount;
            int blockSize = count.syncMode1.blockSize;
            if (blockCount == 0) blockCount = 0x10000;

            if (control.direction == CHCR::Direction::toRam) {
                if (verbose) {
                    fmt::print("[DMA{}] {:<8} -> RAM @ 0x{:08x}, sync, BS: 0x{:04x}, BC: 0x{:04x}\n", (int)channel,
                               magic_enum::enum_name(channel), addr, blockSize, blockCount);
                }
                for (int block = 0; block < blockCount; block++) {
                    for (int i = 0; i < blockSize; i++, addr += 4) {
                        sys->writeMemory32(addr, readDevice());
                    }
                }
            } else if (control.direction == CHCR::Direction::fromRam) {
                if (verbose) {
                    fmt::print("[DMA{}] {:<8} <- RAM @ 0x{:08x}, sync, BS: 0x{:04x}, BC: 0x{:04x}\n", (int)channel,
                               magic_enum::enum_name(channel), addr, blockSize, blockCount);
                }

                for (int block = 0; block < blockCount; block++) {
                    for (int i = 0; i < blockSize; i++, addr += 4) {
                        writeDevice(sys->readMemory32(addr));
                    }
                }
            }
            baseAddress.address = addr;
        } else if (control.syncMode == CHCR::SyncMode::linkedListMode) {
            int addr = baseAddress.address;

            if (verbose) {
                fmt::print("[DMA{}] {:<8} <- RAM @ 0x{:08x}, linked list\n", (int)channel, magic_enum::enum_name(channel), addr);
            }

            int breaker = 0;
            for (;;) {
                uint32_t blockInfo = sys->readMemory32(addr);
                int commandCount = blockInfo >> 24;

                addr += 4;
                for (int i = 0; i < commandCount; i++, addr += 4) {
                    writeDevice(sys->readMemory32(addr));
                }
                addr = blockInfo & 0xffffff;
                if (addr == 0xffffff || addr == 0) break;

                if (++breaker > 0x4000) {
                    fmt::print("[DMA{}] GPU DMA transfer too long, breaking.\n", (int)channel);
                    break;
                }
            }
            baseAddress.address = addr;
        }

        irqFlag = true;
        control.enabled = CHCR::Enabled::completed;
    }
}
}  // namespace device::dma
