/**
 * Controller - handle all communication between peripherials such
 * as digital controller, analog controller, mouse and memory card.
 */
#pragma once
#include <array>
#include <memory>
#include <string>
#include "device/device.h"
#include "peripherals/memory_card.h"
#include "utils/timing.h"

struct System;

namespace device {
namespace controller {
union Mode {
    enum class BaudrateReloadFactor : uint16_t { mul1 = 1, mul16 = 2, mul64 = 3, _mul1 = 0 };
    enum class Bits : uint16_t { bit5 = 0, bit6 = 1, bit7 = 2, bit8 = 3 };
    enum class Parity : uint16_t { even = 0, odd = 1 };
    enum class ClockIdlePolarity : uint16_t { high = 0, low = 1 };
    struct {
        BaudrateReloadFactor baudrateReloadFactor : 2;
        Bits bits : 2;
        uint16_t parityEnable : 1;
        Parity parity : 1;
        uint16_t : 2;
        ClockIdlePolarity clockIdlePolarity : 1;
        uint16_t : 7;
    };
    uint16_t _reg;
    uint8_t _byte[2];
    Mode() : _reg(0) {}

    int getPrescaler() const {
        switch (baudrateReloadFactor) {
            case BaudrateReloadFactor::mul1:
            case BaudrateReloadFactor::_mul1: return 1;
            case BaudrateReloadFactor::mul16: return 16;
            case BaudrateReloadFactor::mul64: return 64;
            default: return 1;  // Unreachable
        }
    }
    int getBits() const {
        switch (bits) {
            case Bits::bit5: return 5;
            case Bits::bit6: return 6;
            case Bits::bit7: return 7;
            case Bits::bit8: return 8;
            default: return 8;  // Unreachable
        }
    }
};
union Status {
    struct {
        uint32_t txReady : 1;
        uint32_t rxPending : 1;
        uint32_t txFinished : 1;
        uint32_t rxParityError : 1;
        uint32_t : 3;
        uint32_t ack : 1;
        uint32_t : 1;
        uint32_t irq : 1;
        uint32_t : 1;
        uint32_t baudTimer : 21;
    };
    uint32_t _reg;
    uint8_t _byte[4];
    Status() : _reg(0) {}
};
union Control {
    struct {
        uint16_t txEnable : 1;
        uint16_t select : 1;  // /SEL
        uint16_t rxEnable : 1;
        uint16_t : 1;

        uint16_t acknowledge : 1;
        uint16_t : 1;
        uint16_t reset : 1;
        uint16_t : 1;  // Always 0

        uint16_t rxInterruptMode : 2;    // irq when fifo has n bytes (n: 0 - 1, 1 - 2, 2 - 4, 3 - 8)
        uint16_t txInterruptEnable : 1;  // when stat 0 or 2
        uint16_t rxInterruptEnable : 1;  // irq when fifo has n bytes
        uint16_t ackInterruptEnable : 1;

        uint16_t port : 1;
        uint16_t : 3;
    };
    uint16_t _reg;
    uint8_t _byte[2];
    Control() : _reg(0) {}
};

enum class DeviceSelected { None, Controller, MemoryCard };

class Controller {
    int verbose;
    int busToken;
    System* sys;

    DeviceSelected deviceSelected;

    Mode mode;
    Control control;
    Reg16 baud;
    Status status;
    int rxDelay = 0;      // CPU cycles
    int ackDelay = 0;     // CPU cycles
    int ackDuration = 0;  // CPU cycles
    uint8_t rxData = 0;

    void handleByte(uint8_t byte);
    uint8_t getData();
    void postByte(uint8_t data);

    // Measured delay was between 6.8us and 13.7us for controller
    // ~5us for memory card for fast response (first responses to read command)
    // 31.2us for slow response (address processing and data read)
    // Duration was 2.84us (controller) and 2.2us (memory card)
    void postAck(int delayUs = 6, int durationUs = 2);

   public:
    std::array<std::unique_ptr<peripherals::AbstractDevice>, 2> controller;
    std::array<std::unique_ptr<peripherals::MemoryCard>, 2> card;

    Controller(System* sys);
    ~Controller();
    void reset();
    void reload();
    void step(int cpuCycles);
    uint8_t read(uint32_t address);
    void write(uint32_t address, uint8_t data);
    void update();

    template <class Archive>
    void serialize(Archive& ar) {
        ar(deviceSelected, mode._reg, control._reg, baud._reg, status._reg, rxDelay, ackDelay, ackDuration, rxData);
        // TODO: serialize controller && card state
    }
};
}  // namespace controller
}  // namespace device
