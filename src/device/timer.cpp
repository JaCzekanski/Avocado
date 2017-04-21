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
            current._reg += cnt / 2;
            cnt %= 2;
        }
    } else if (which == 1) {
        if (mode.clockSource.clockSource1 == CounterMode::ClockSource1::hblank
            || mode.clockSource.clockSource1 == CounterMode::ClockSource1::hblank_) {
            current._reg += cnt / 3413;
            cnt %= 3413;
        } else {  // System Clock
            current._reg += cnt / 2;
            cnt %= 2;
        }
    } else if (which == 2) {
        if (mode.clockSource.clockSource2 == CounterMode::ClockSource2::systemClock_8
            || mode.clockSource.clockSource2 == CounterMode::ClockSource2::systemClock_8_) {
            current._reg += cnt / (8 * 2);
            cnt %= 8 * 2;
        } else {  // System Clock
            current._reg += cnt / 2;
            cnt %= 2;
        }
    }

    if (current._reg == target._reg) {
        if (mode.irqWhenTarget) {
            mode.interruptRequest = false;
            static_cast<mips::CPU*>(_cpu)->interrupt->IRQ(mapIrqNumber());
        }
        if (mode.resetToZero == CounterMode::ResetToZero::whenTarget) current._reg = 0;
        mode.reachedTarget = true;
    } else if (current._reg == 0xffff) {
        if (mode.irqWhenFFFF) {
            mode.interruptRequest = false;
            static_cast<mips::CPU*>(_cpu)->interrupt->IRQ(mapIrqNumber());
        }
        if (mode.resetToZero == CounterMode::ResetToZero::whenFFFF) current._reg = 0;
        mode.reachedFFFF = true;
    }
}
uint8_t Timer::read(uint32_t address) {
    if (address < 4) {
        return current.read(address);
    } else if (address >= 4 && address < 8) {
        uint8_t v = mode.read(address - 4);
        mode.reachedFFFF = false;
        mode.reachedTarget = false;
        return v;
    } else if (address >= 8 && address < 12) {
        return target.read(address - 8);
    }
    return 0;
}
void Timer::write(uint32_t address, uint8_t data) {
    if (address < 4) {
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
