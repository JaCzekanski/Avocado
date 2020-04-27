#include "dma_channel.h"
#include <fmt/core.h>
#include <magic_enum.hpp>
#include "config.h"
#include "system.h"
#include <unordered_set>

namespace device::dma {
DMAChannel::DMAChannel(Channel channel, System* sys) : channel(channel), sys(sys) { verbose = config.debug.log.dma; }

DMAChannel::~DMAChannel() {}

uint32_t DMAChannel::readDevice() { return 0; }

void DMAChannel::writeDevice(uint32_t data) { (void)data; }

uint8_t DMAChannel::read(uint32_t address) {
    if (address < 0x4) return baseAddress._byte[address];
    if (address >= 0x4 && address < 0x8) return count._byte[address - 4];
    if (address >= 0x8 && address < 0xc) return control._byte[address - 8];
    return 0;
}

void DMAChannel::write(uint32_t address, uint8_t data) {
    if (address < 0x4) {
        baseAddress._byte[address] = data;
        baseAddress._reg &= 0xffffff;
    } else if (address >= 0x4 && address < 0x8) {
        count._byte[address - 4] = data;
    } else if (address >= 0x8 && address < 0xc) {
        control._byte[address - 8] = data;
        maskControl();

        if (address == 0xb) {
            if (!sys->dma->isChannelEnabled(channel)) return;
            if (control.syncMode == CHCR::SyncMode::block && control.startTrigger != CHCR::StartTrigger::manual) return;
            if (control.enabled != CHCR::Enabled::start) return;

            startTransfer();
        }
    }
}

void DMAChannel::maskControl() { control._reg &= ~CHCR::MASK; }

void DMAChannel::startTransfer() {
    using namespace magic_enum;

    int step = control.memoryAddressStep == CHCR::MemoryAddressStep::forward ? 4 : -4;
    uint32_t addr = baseAddress.address;

    control.startTrigger = CHCR::StartTrigger::clear;

    if (control.syncMode == CHCR::SyncMode::block) {
        if (verbose) {
            fmt::print("[DMA{}] {:<8} {} RAM @ 0x{:08x}, {}, count: 0x{:04x}\n", (int)channel, enum_name(channel), control.dir(), addr,
                       enum_name(control.syncMode), (int)count.syncMode0.wordCount);
        }
        if (control.direction == CHCR::Direction::fromRam) {
            for (size_t i = 0; i < count.syncMode0.wordCount; i++, addr += step) {
                writeDevice(sys->readMemory32(addr));
            }
        } else if (control.direction == CHCR::Direction::toRam) {
            for (size_t i = 0; i < count.syncMode0.wordCount; i++, addr += step) {
                sys->writeMemory32(addr, readDevice());
            }
        }
        control.enabled = CHCR::Enabled::stop;

        sys->cpuStalledCycles += count.syncMode0.wordCount * 4;  // Hardcoded RAM access time
        sys->cpuStalledCycles += count.syncMode0.wordCount * 2;  // Hardcoded Device access time
    } else if (control.syncMode == CHCR::SyncMode::sync) {
        int blockCount = count.syncMode1.blockCount;
        int blockSize = count.syncMode1.blockSize;
        if (blockCount == 0) blockCount = 0x10000;

        if (verbose) {
            fmt::print("[DMA{}] {:<8} {} RAM @ 0x{:08x}, {}, BS: 0x{:04x}, BC: 0x{:04x}\n", (int)channel, enum_name(channel), control.dir(),
                       addr, enum_name(control.syncMode), blockSize, blockCount);
        }

        // TODO: Execute sync with chopping
        if (control.direction == CHCR::Direction::toRam) {
            for (int block = 0; block < blockCount; block++) {
                for (int i = 0; i < blockSize; i++, addr += step) {
                    sys->writeMemory32(addr, readDevice());
                }
            }
        } else if (control.direction == CHCR::Direction::fromRam) {
            for (int block = 0; block < blockCount; block++) {
                for (int i = 0; i < blockSize; i++, addr += step) {
                    writeDevice(sys->readMemory32(addr));
                }
            }
        }
        // TODO: Need proper Chopping implementation for SPU READ to work
        baseAddress.address = addr;
        count.syncMode1.blockCount = 0;

        sys->cpuStalledCycles += blockCount * blockSize / 4 * 4;  // Hardcoded RAM access time
        sys->cpuStalledCycles += blockCount * blockSize / 4 * 2;  // Hardcoded Device access time
    } else if (control.syncMode == CHCR::SyncMode::linkedList) {
        if (verbose) {
            fmt::print("[DMA{}] {:<8} {} RAM @ 0x{:08x}, {}\n", (int)channel, enum_name(channel), control.dir(), addr,
                       enum_name(control.syncMode));
        }

        std::unordered_set<uint32_t> visited;
        int wordsRead = 0;
        for (;; wordsRead++) {
            uint32_t blockInfo = sys->readMemory32(addr);
            int commandCount = blockInfo >> 24;
            int nextAddr = blockInfo & 0xffffff;

            if (verbose >= 2) {
                fmt::print("[DMA{}] {:<8} {} RAM @ 0x{:08x}, {}, count: {}, nextAddr: 0x{:08x}\n", (int)channel, enum_name(channel),
                           control.dir(), addr, enum_name(control.syncMode), commandCount, nextAddr);
            }

            addr += step;
            for (int i = 0; i < commandCount; i++, addr += step, wordsRead++) {
                writeDevice(sys->readMemory32(addr));
            }

            addr = nextAddr;
            if (addr == 0xffffff || addr == 0) break;

            if (visited.find(addr) != visited.end()) {
                fmt::print("[DMA{}] GPU DMA transfer loop detected, breaking.\n", (int)channel);
                break;
            }
            visited.insert(addr);
        }
        baseAddress.address = addr;

        sys->cpuStalledCycles += wordsRead * 4;  // Hardcoded RAM access time
        sys->cpuStalledCycles += wordsRead * 2;  // Hardcoded Device access time
    }

    irqFlag = true;
    control.enabled = CHCR::Enabled::completed;
}
}  // namespace device::dma
