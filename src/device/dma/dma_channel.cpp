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
static bool canLog = true;
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
            canLog = true;
            step();
        }
    }
}

void DMAChannel::step() {
    if (!sys->dma->isChannelEnabled(channel)) return;
    if (control.enabled != CHCR::Enabled::start) return;
    if (control.syncMode == CHCR::SyncMode::block && control.startTrigger != CHCR::StartTrigger::manual) return;

    control.startTrigger = CHCR::StartTrigger::clear;

    if (control.syncMode == CHCR::SyncMode::block) {
        burstTransfer();
    } else if (control.syncMode == CHCR::SyncMode::sync) {
        syncBlockTransfer();
    } else if (control.syncMode == CHCR::SyncMode::linkedList) {
        linkedListTransfer();
    }
}

void DMAChannel::maskControl() { control._reg &= ~CHCR::MASK; }

// Sync 0 - no cpu execution in between
void DMAChannel::burstTransfer() {
    uint32_t addr = baseAddress.address;

    uint32_t wordCount = count.syncMode0.wordCount;
    if (wordCount == 0) wordCount = 0x10000;

    if (verbose && canLog) {
        fmt::print("[DMA{}] {:<8} {} RAM @ 0x{:08x}, {}, count: 0x{:04x}\n", (int)channel, magic_enum::enum_name(channel), control.dir(),
                   addr, magic_enum::enum_name(control.syncMode), wordCount);
        canLog = false;
    }
    if (control.direction == CHCR::Direction::toRam) {
        for (size_t i = 0; i < wordCount; i++, addr += control.step()) {
            sys->writeMemory32(addr, readDevice());
        }
    } else if (control.direction == CHCR::Direction::fromRam) {
        for (size_t i = 0; i < wordCount; i++, addr += control.step()) {
            writeDevice(sys->readMemory32(addr));
        }
    }

    irqFlag = true;
    control.enabled = CHCR::Enabled::completed;
}

void DMAChannel::syncBlockTransfer() {
    if (!dataRequest()) return;

    uint32_t addr = baseAddress.address;

    if (verbose && canLog) {
        fmt::print("[DMA{}] {:<8} {} RAM @ 0x{:08x}, {}, BS: 0x{:04x}, BC: 0x{:04x}\n", (int)channel, magic_enum::enum_name(channel),
                   control.dir(), addr, magic_enum::enum_name(control.syncMode), (int)count.syncMode1.blockSize,
                   (int)count.syncMode1.blockCount);
        canLog = false;
    }

    // TODO: DREQ DACK for MDEC
    // TODO: Execute sync with chopping
    if (control.direction == CHCR::Direction::toRam) {
        for (int i = 0; i < count.syncMode1.blockSize; i++, addr += control.step()) {
            sys->writeMemory32(addr, readDevice());
        }
    } else if (control.direction == CHCR::Direction::fromRam) {
        for (int i = 0; i < count.syncMode1.blockSize; i++, addr += control.step()) {
            writeDevice(sys->readMemory32(addr));
        }
    }
    // TODO: Need proper Chopping implementation for SPU READ to work

    baseAddress.address = addr;
    if (--count.syncMode1.blockCount == 0) {
        irqFlag = true;
        control.enabled = CHCR::Enabled::completed;
    }
}

void DMAChannel::linkedListTransfer() {
    uint32_t addr = baseAddress.address;

    if (verbose && canLog) {
        fmt::print("[DMA{}] {:<8} {} RAM @ 0x{:08x}, {}\n", (int)channel, magic_enum::enum_name(channel), control.dir(), addr,
                   magic_enum::enum_name(control.syncMode));
        canLog = false;
    }

    // TODO: Break execution in between
    std::unordered_set<uint32_t> visited;
    for (;;) {
        uint32_t blockInfo = sys->readMemory32(addr);
        int commandCount = blockInfo >> 24;
        int nextAddr = blockInfo & 0xffffff;

        if (verbose >= 2) {
            fmt::print("[DMA{}] {:<8} {} RAM @ 0x{:08x}, {}, count: {}, nextAddr: 0x{:08x}\n", (int)channel, magic_enum::enum_name(channel),
                       control.dir(), addr, magic_enum::enum_name(control.syncMode), commandCount, nextAddr);
        }

        addr += control.step();
        for (int i = 0; i < commandCount; i++, addr += control.step()) {
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
    irqFlag = true;
    control.enabled = CHCR::Enabled::completed;
}
}  // namespace device::dma
