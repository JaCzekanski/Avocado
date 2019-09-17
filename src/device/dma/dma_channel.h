#pragma once
#include "device/device.h"

struct System;

namespace device::dma {
enum class Channel { MDECin, MDECout, GPU, CDROM, SPU, PIO, OTC };

// DMA base address
union MADDR {
    struct {
        uint32_t address : 24;
        uint32_t : 8;
    };
    uint32_t _reg;
    uint8_t _byte[4];

    MADDR() : _reg(0) {}
};

// DMA Block Control
union BCR {
    union {
        struct {
            uint32_t wordCount : 16;
            uint32_t : 16;
        } syncMode0;
        struct {
            uint32_t blockSize : 16;
            uint32_t blockCount : 16;
        } syncMode1;
    };
    uint32_t _reg;
    uint8_t _byte[4];

    BCR() : _reg(0) {}
};

// DMA Channel Control
union CHCR {
    enum class Direction : uint32_t { toRam = 0, fromRam = 1 };
    enum class MemoryAddressStep : uint32_t { forward = 0, backward = 1 };
    enum class SyncMode : uint32_t { startImmediately = 0, syncBlockToDmaRequests = 1, linkedListMode = 2 };
    enum class ChoppingEnable : uint32_t { normal = 0, chopping = 1 };
    enum class Enabled : uint32_t { completed = 0, stop = 0, start = 1 };
    enum class StartTrigger : uint32_t { clear = 0, automatic = 0, manual = 1 };

    struct {
        Direction direction : 1;
        MemoryAddressStep memoryAddressStep : 1;
        uint32_t : 6;
        ChoppingEnable choppingEnable : 1;
        SyncMode syncMode : 2;
        uint32_t : 5;
        uint32_t choppingDmaWindowSize : 3;  // Chopping DMA Window Size (1 SHL N words)
        uint32_t : 1;
        uint32_t choppingCpuWindowSize : 3;  // Chopping CPU Window Size(1 SHL N clks)
        uint32_t : 1;
        Enabled enabled : 1;  // stopped/completed, start/enable/busy
        uint32_t : 3;
        StartTrigger startTrigger : 1;
        uint32_t : 3;
    };
    uint32_t _reg;
    uint8_t _byte[4];

    CHCR() : _reg(0) {}
};

class DMAChannel {
    int verbose;
    Channel channel;

    CHCR control;
    MADDR baseAddress;
    BCR count;

    System* sys;

    virtual uint32_t readDevice();
    virtual void writeDevice(uint32_t data);

   public:
    bool irqFlag = false;
    DMAChannel(Channel channel, System* sys);
    virtual ~DMAChannel();

    uint8_t read(uint32_t address);
    void write(uint32_t address, uint8_t data);

    template <class Archive>
    void serialize(Archive& ar) {
        ar(control._reg, baseAddress._reg, count._reg);
    }
};
}  // namespace device::dma
