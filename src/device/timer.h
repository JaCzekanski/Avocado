#pragma once
#include <cassert>
#include <cereal/access.hpp>
#include "interrupt.h"

namespace gui::debug {
class Timers;
}

namespace device::timer {
union CounterMode {
    enum class SyncMode0 : uint16_t {
        pauseDuringHblank = 0,
        resetAtHblank = 1,
        resetAtHblankAndPauseOutside = 2,
        pauseUntilHblankAndFreerun = 3
    };
    enum class SyncMode1 : uint16_t {
        pauseDuringVblank = 0,
        resetAtVblank = 1,
        resetAtVblankAndPauseOutside = 2,
        pauseUntilVblankAndFreerun = 3
    };
    enum class SyncMode2 : uint16_t {  //
        stopCounter = 0,
        freeRun = 1,
        freeRun_ = 2,
        stopCounter_ = 3
    };

    enum class ResetToZero : uint16_t { whenFFFF = 0, whenTarget = 1 };
    enum class IrqRepeatMode : uint16_t { oneShot = 0, repeatedly = 1 };
    enum class IrqPulseMode : uint16_t { shortPulse = 0, toggle = 1 };
    enum class ClockSource0 : uint16_t { systemClock = 0, dotClock = 1 };
    enum class ClockSource1 : uint16_t { systemClock = 0, hblank = 1 };
    enum class ClockSource2 : uint16_t { systemClock = 0, systemClock_8 = 1 };

    struct {
        uint16_t syncEnabled : 1;
        uint16_t syncMode : 2;
        ResetToZero resetToZero : 1;

        uint16_t irqWhenTarget : 1;
        uint16_t irqWhenFFFF : 1;
        IrqRepeatMode irqRepeatMode : 1;
        IrqPulseMode irqPulseMode : 1;

        // For all timer different clock sources are available
        uint16_t clockSource : 2;
        uint16_t interruptRequest : 1;  // R
        uint16_t reachedTarget : 1;     // R
        uint16_t reachedFFFF : 1;       // R

        uint16_t : 3;
    };

    uint16_t _reg;
    uint8_t _byte[2];

    CounterMode() : _reg(0) { interruptRequest = 1; }
    void write(int n, uint8_t v) {
        if (n >= 2) return;
        _byte[n] = v;
    }
    uint8_t read(int n) const {
        if (n >= 2) return 0;
        return _byte[n];
    }
};

class Timer {
    friend class gui::debug::Timers;
    friend class cereal::access;

    int which;

   public:
    Reg16 current;
    CounterMode mode;
    Reg16 target;

    bool paused = false;
    uint32_t cnt = 0;

   private:
    bool oneShotIrqOccured = false;

    System* sys;

    void checkIrq();
    interrupt::IrqNumber mapIrqNumber() const {
        if (which == 0) return interrupt::TIMER0;
        if (which == 1) return interrupt::TIMER1;
        if (which == 2) return interrupt::TIMER2;
        assert(false);
        return interrupt::TIMER0;
    }

   public:
    Timer(System* sys, int which);
    void step(int cycles);
    uint8_t read(uint32_t address);
    void write(uint32_t address, uint8_t data);

    template <class Archive>
    void serialize(Archive& ar) {
        ar(current, mode._reg, target, cnt, oneShotIrqOccured);
    }
};
};  // namespace device::timer