#pragma once
#include "device.h"

namespace device {
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
    enum class ClockSource0 : uint32_t {
        systemClock = 0,
        systemClock_ = 2,
        dotClock = 1,
        dotClock_ = 3,
    };
    enum class ClockSource1 : uint32_t {
        systemClock = 0,
        systemClock_ = 2,
        hblank = 1,
        hblank_ = 3,
    };
    enum class ClockSource2 : uint32_t {
        systemClock = 0,
        systemClock_ = 1,
        systemClock_8 = 2,
        systemClock_8_ = 3,
    };

    struct {
        SynchronizationEnable synchronizationEnable : 1;
        union {
            SynchronizationMode0 synchronizationMode0 : 2;
            SynchronizationMode1 synchronizationMode1 : 2;
            SynchronizationMode2 synchronizationMode2 : 2;
        } synchronizationMode;

        ResetToZero resetToZero : 1;
        Bit irqWhenTarget : 1;
        Bit irqWhenFFFF : 1;
        IrqRepeatMode irqRepeatMode : 1;
        IrqPulseMode irqPulseMode : 1;

        // For all timer different clock sources are available
        union {
            ClockSource0 clockSource0 : 2;
            ClockSource1 clockSource1 : 2;
            ClockSource2 clockSource2 : 2;
        } clockSource;

        Bit interruptRequest : 1;  // R
        Bit reachedTarget : 1;     // R
        Bit reachedFFFF : 1;       // R

        uint32_t : 2;
        uint32_t : 16;
    };

    uint32_t _reg;
    uint8_t _byte[4];

    CounterMode() : _reg(0) {}
};

class Timer : public Device {
    const int baseAddress = 0x1f801100;

    int which = 0;
    uint16_t current = 0;
    CounterMode mode;
    uint16_t target = 0;

    int cnt = 0;

    void *_cpu = nullptr;

   public:
    Timer(int which);
    void step();
    uint8_t read(uint32_t address);
    void write(uint32_t address, uint8_t data);
    void setCPU(void *cpu) { this->_cpu = cpu; }
};
}
}
