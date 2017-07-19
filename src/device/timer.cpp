#include "timer.h"
#include <cstdio>
#include "../mips.h"

namespace device {
namespace timer {
Timer::Timer(int which) : which(which) {}
void Timer::step(int cycles) {
    cnt += cycles;

    if (which == 0) {
        if (mode.clockSource.clockSource0 == CounterMode::ClockSource0::dotClock
            || mode.clockSource.clockSource0 == CounterMode::ClockSource0::dotClock_) {
            current._reg += cnt;
            cnt = 0;
        } else {  // System Clock
            current._reg += cnt / 3;
            cnt %= 3;
        }
    } else if (which == 1) {
        if (mode.clockSource.clockSource1 == CounterMode::ClockSource1::hblank
            || mode.clockSource.clockSource1 == CounterMode::ClockSource1::hblank_) {
            current._reg += cnt / 3413;
            cnt %= 3413;
        } else {  // System Clock
            current._reg += cnt / 3;
            cnt %= 3;
        }
    } else if (which == 2) {
        if (mode.clockSource.clockSource2 == CounterMode::ClockSource2::systemClock_8
            || mode.clockSource.clockSource2 == CounterMode::ClockSource2::systemClock_8_) {
            current._reg += cnt / (8 * 3);
            cnt %= 8 * 3;
        } else {  // System Clock
            current._reg += cnt / 3;
            cnt %= 3;
        }
    }

    if (current._reg >= target._reg) {
        if (mode.resetToZero == CounterMode::ResetToZero::whenTarget) {
            current._reg = 0;
        }
        if (mode.irqWhenTarget) {
            mode.interruptRequest = false;
            static_cast<mips::CPU*>(_cpu)->interrupt->trigger(mapIrqNumber());
        }
        mode.reachedTarget = true;
    }

    if (current._reg >= 0xffff) {
        if (mode.resetToZero == CounterMode::ResetToZero::whenFFFF) {
            current._reg = 0;
        }
        if (mode.irqWhenFFFF) {
            mode.interruptRequest = false;
            static_cast<mips::CPU*>(_cpu)->interrupt->trigger(mapIrqNumber());
        }
        mode.reachedFFFF = true;
    }
}
uint8_t Timer::read(uint32_t address) {
    if (address < 2) {
        return current.read(address);
    } else if (address >= 4 && address < 8) {
        uint8_t v = mode.read(address - 4);
        if (address == 7) {
            mode.reachedFFFF = false;
            mode.reachedTarget = false;
        }
        return v;
    } else if (address >= 8 && address < 12) {
        return target.read(address - 8);
    }
    return 0;
}
void Timer::write(uint32_t address, uint8_t data) {
    if (address < 2) {
        current.write(address, data);
    } else if (address >= 4 && address < 8) {
        current._reg = 0;
        mode.write(address - 4, data);  // BIOS uses 0x0148 for TIMER1
    } else if (address >= 8 && address < 12) {
        target.write(address - 8, data);
    }
}
}
}
