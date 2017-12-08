#pragma once
#include "device.h"
#include "interrupt.h"
#include <cassert>

namespace timer {
union CounterMode {
    enum class SynchronizationEnable { freeRun = 0, synchronize = 1 };
    enum class SynchronizationMode0 {
        pauseCounterDuringHblanks = 0,
        resetCounterAtHblanks = 1,
        resetCounterAtHblanksAndPauseOutside = 2,
        pauseUntilHblanksOnceAndFreerun = 3
    };
    enum class SynchronizationMode1 {
        pauseCounterDuringVblanks = 0,
        resetCounterAtVblanks = 1,
        resetCounterAtVblanksAndPauseOutside = 2,
        pauseUntilVblanksOnceAndFreerun = 3
    };
    enum class SynchronizationMode2 {
        stopCounterAtCurrentValue = 0,  // Stop counter at current value (no h/v-blank start)
        freeRun = 1,
        stopCounterAtCurrentValue_ = 2,
        freeRun_ = 3
    };

    enum class ResetToZero : uint32_t { whenFFFF = 0, whenTarget = 1 };
    enum class IrqRepeatMode : uint32_t { oneShot = 0, repeatedly = 1 };
    enum class IrqPulseMode : uint32_t {
        shortPulse = 0,  // Short Bit10 = 0 pulse
        toggle = 1       // Toggle Bit10 on/off
    };
    enum class ClockSource0 : uint32_t { systemClock = 0, dotClock = 1 };
    enum class ClockSource1 : uint32_t { systemClock = 0, hblank = 1 };
    enum class ClockSource2 : uint32_t { systemClock = 0, systemClock_8 = 1 };

    struct {
        SynchronizationEnable synchronizationEnable : 1;
        union {
            SynchronizationMode0 synchronizationMode0 : 2;
            SynchronizationMode1 synchronizationMode1 : 2;
            SynchronizationMode2 synchronizationMode2 : 2;
        } synchronizationMode;

        ResetToZero resetToZero : 1;
        uint32_t irqWhenTarget : 1;
        uint32_t irqWhenFFFF : 1;
        IrqRepeatMode irqRepeatMode : 1;
        IrqPulseMode irqPulseMode : 1;

        // For all timer different clock sources are available
        union {
            ClockSource0 clockSource0 : 1;
            ClockSource1 clockSource1 : 1;
        };
        ClockSource2 clockSource2 : 1;

        Bit interruptRequest : 1;  // R
        Bit reachedTarget : 1;     // R
        Bit reachedFFFF : 1;       // R

        uint32_t : 2;
        uint32_t : 16;
    };

    uint32_t _reg;
    uint8_t _byte[4];

    CounterMode() : _reg(0) {}
    void write(int n, uint8_t v) {
        if (n >= 4) return;
        _byte[n] = v;
    }
    uint8_t read(int n) const {
        if (n >= 4) return 0;
        return _byte[n];
    }
};
}

class Timer {
    const int baseAddress = 0x1f801100;

    int which = 0;

   public:
    Reg32 current;
    timer::CounterMode mode;
    Reg16 target;

   private:
    int cnt = 0;
    bool irqOccured = false;

    mips::CPU* cpu = nullptr;

    interrupt::IrqNumber mapIrqNumber() const {
        if (which == 0) return interrupt::TIMER0;
        if (which == 1) return interrupt::TIMER1;
        if (which == 2) return interrupt::TIMER2;
        assert(false);
        return interrupt::TIMER0;
    }

   public:
    Timer(mips::CPU* cpu, int which);
    void step() { step(1); }
    void step(int cycles);
    uint8_t read(uint32_t address);
    void write(uint32_t address, uint8_t data);
};