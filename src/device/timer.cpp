#include "timer.h"
#include <cstdio>
#include "../mips.h"

namespace device {
namespace timer {
Timer::Timer(int which) : which(which) {}
void Timer::step() {
    static int cnt = 0;

    ++cnt;

    if (which == 0) {
        if (mode.clockSource.clockSource0 == CounterMode::ClockSource0::dotClock
            || mode.clockSource.clockSource0 == CounterMode::ClockSource0::dotClock_) {
            cnt = 0;
            current++;
        } else {  // System Clock
            if (cnt >= 2) {
                cnt = 0;
                current++;
            }
        }
    } else if (which == 1) {
        if (mode.clockSource.clockSource1 == CounterMode::ClockSource1::hblank
            || mode.clockSource.clockSource1 == CounterMode::ClockSource1::hblank_) {
            if (cnt >= 3412) {
                cnt = 0;
                current++;
            }
        } else {  // System Clock
            if (cnt >= 2) {
                cnt = 0;
                current++;
            }
        }
    } else if (which == 2) {
        if (mode.clockSource.clockSource2 == CounterMode::ClockSource2::systemClock_8
            || mode.clockSource.clockSource2 == CounterMode::ClockSource2::systemClock_8_) {
            if (cnt >= 16) {
                cnt = 0;
                current++;
            }
        } else {  // System Clock
            if (cnt >= 2) {
                cnt = 0;
                current++;
            }
        }
    }

    if (current == target) {
        if (mode.irqWhenTarget) static_cast<mips::CPU*>(_cpu)->interrupt->IRQ(4 + which);
        if (mode.resetToZero == CounterMode::ResetToZero::whenTarget) current = 0;
        mode.reachedTarget = true;
    } else if (current == 0xffff) {
        if (mode.irqWhenFFFF) static_cast<mips::CPU*>(_cpu)->interrupt->IRQ(4 + which);
        if (mode.resetToZero == CounterMode::ResetToZero::whenFFFF) current = 0;
        mode.reachedFFFF = true;
    }
}
uint8_t Timer::read(uint32_t address) {
    if (address < 4) return current >> (address * 8);
    if (address >= 4 && address < 8) return mode._byte[address];
    if (address >= 8 && address < 12) return target >> ((address - 8) * 8);
    return 0;
}
void Timer::write(uint32_t address, uint8_t data) {
    if (address < 4) {
        current &= (0xff) << (address * 8);
        current |= data << (address * 8);
        return;
    }
    if (address >= 4 && address < 8) {
        mode._byte[address - 4] = data;
        if (address - 4 == 1) {
            mode.reachedFFFF = false;
            mode.reachedTarget = false;
        }
        return;
    }
    if (address >= 8 && address < 12) {
        target &= (0xff) << ((address - 8) * 8);
        target |= data << ((address - 8) * 8);
    }
}
}
}
