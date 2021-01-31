#pragma once
#include <cassert>
#include "interrupt.h"
#include "utils/timing.h"

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

    int which;

   public:
    Reg16 current;
    CounterMode mode;
    Reg16 target;

    bool paused = false;
    uint32_t cnt = 0;

   private:
    bool oneShotIrqOccured = false;
    int customPeriod = 5;

    System* sys;

    void checkIrq();
    interrupt::IrqNumber mapIrqNumber() const {
        if (which == 0)
            return interrupt::TIMER0;
        else if (which == 1)
            return interrupt::TIMER1;
        else if (which == 2)
            return interrupt::TIMER2;
        else
            __builtin_unreachable();
    }

   public:
    Timer(System* sys, int which);
    void step(int cycles);
    uint8_t read(uint32_t address);
    void write(uint32_t address, uint8_t data);

    void updateDotclockPeriod(int horizontalResolution) {
        if (which != 0) return;

        int step;
        switch (horizontalResolution) {
            case 256: step = timing::GPU_CLOCK / 10; break;
            case 320: step = timing::GPU_CLOCK / 8; break;
            case 368: step = timing::GPU_CLOCK / 7; break;
            case 512: step = timing::GPU_CLOCK / 5; break;
            case 640: step = timing::GPU_CLOCK / 4; break;
            default: __builtin_unreachable();
        }
        customPeriod = roundf(timing::CPU_CLOCK / step);
    }

    void updateHblankPeriod(bool isNtsc) {
        if (which != 1) return;

        if (isNtsc) {
            customPeriod = roundf(timing::CYCLES_PER_LINE_NTSC * 7 / 11);
        } else {
            customPeriod = roundf(timing::CYCLES_PER_LINE_PAL * 7 / 11);
        }
    }

    template <class Archive>
    void serialize(Archive& ar) {
        ar(current, mode._reg, target, cnt, oneShotIrqOccured, customPeriod);
    }
};
};  // namespace device::timer