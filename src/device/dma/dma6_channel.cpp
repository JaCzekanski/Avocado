#include "dma6_channel.h"
#include <fmt/core.h>
#include <magic_enum.hpp>
#include "system.h"

namespace device::dma {
DMA6Channel::DMA6Channel(Channel channel, System* sys) : DMAChannel(channel, sys) {}

void DMA6Channel::maskControl() {
    control._reg &= ~CHCR::MASK;
    control.direction = CHCR::Direction::toRam;
    control.syncMode = CHCR::SyncMode::block;
    control.memoryAddressStep = CHCR::MemoryAddressStep::backward;
    control.choppingEnable = false;
    control.choppingDmaWindowSize = 0;
    control.choppingCpuWindowSize = 0;
    control.unknown1 = false;
}

void DMA6Channel::startTransfer() {
    control.startTrigger = CHCR::StartTrigger::clear;
    uint32_t addr = baseAddress.address;
    uint32_t wordCount = count.syncMode0.wordCount;
    if (wordCount == 0) {
        wordCount = 0x10000;
    }

    if (verbose) {
        fmt::print("[DMA{}] {:<8} -> RAM @ 0x{:08x}, block, count: 0x{:04x}\n", (int)channel, magic_enum::enum_name(channel), addr,
                   wordCount);
    }

    for (int i = wordCount - 1; i >= 0; i--, addr -= 4) {
        sys->writeMemory32(addr, (i == 0) ? 0xffffff : (addr - 4) & 0xffffff);
    }

    irqFlag = true;
    control.enabled = CHCR::Enabled::completed;
}
}  // namespace device::dma