#include "timer.h"
#include <cstdio>
#include "../mips.h"

namespace device {
namespace timer {
Timer::Timer(int which) : which(which) {}
void Timer::step() {
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
            // if (cnt >= 3412/4) {
            //    cnt = 0;
            current++;
            //}
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
        if (mode.irqWhenTarget) {
            mode.interruptRequest = false;
            static_cast<mips::CPU*>(_cpu)->interrupt->IRQ(4 + which);
        }
        if (mode.resetToZero == CounterMode::ResetToZero::whenTarget) current = 0;
        mode.reachedTarget = true;
    } else if (current == 0xffff) {
        if (mode.irqWhenFFFF) {
            mode.interruptRequest = false;
            static_cast<mips::CPU*>(_cpu)->interrupt->IRQ(4 + which);
        }
        if (mode.resetToZero == CounterMode::ResetToZero::whenFFFF) current = 0;
        mode.reachedFFFF = true;
    }
}
uint8_t Timer::read(uint32_t address) {
    if (address < 2) return current >> (address * 8);
    if (address >= 4 && address < 6) return mode._byte[address - 4];
    if (address >= 8 && address < 10) {
        uint8_t ret = target >> ((address - 8) * 8);

        if (address == 9) {
            mode.reachedFFFF = false;
            mode.reachedTarget = false;
        }
        return ret;
    }
    return 0;
}
void Timer::write(uint32_t address, uint8_t data) {
    if (address < 2) {
        current &= ~((0xff) << (address * 8));
        current |= data << (address * 8);
        return;
    }
    if (address >= 4 && address < 6) {
        mode._byte[address - 4] = data;
        if (address - 4 == 1) {
            mode.reachedFFFF = false;
            mode.reachedTarget = false;
        }
        return;
    }
    if (address >= 8 && address < 10) {
        target &= ~((0xff) << ((address - 8) * 8));
        target |= data << ((address - 8) * 8);
    }
}
}
}
