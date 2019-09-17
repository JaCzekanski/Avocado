#include "timer.h"
#include <fmt/core.h>
#include "system.h"

namespace device::timer {

Timer::Timer(System* sys, int which) : which(which), sys(sys) {}

void Timer::step(int cycles) {
    if (paused) return;
    cnt += cycles;

    uint32_t tval = current._reg;
    if (which == 0) {
        auto clock = static_cast<CounterMode::ClockSource0>(mode.clockSource & 1);
        using modes = CounterMode::ClockSource0;

        if (clock == modes::dotClock) {
            tval += cnt / 6;
            cnt %= 6;
        } else {  // System Clock
            tval += (int)(cnt / 1.5f);
            cnt %= (int)1.5f;
        }
    } else if (which == 1) {
        auto clock = static_cast<CounterMode::ClockSource1>(mode.clockSource & 1);
        using modes = CounterMode::ClockSource1;

        if (clock == modes::hblank) {
            tval += cnt / 3413;
            cnt %= 3413;
        } else {  // System Clock
            tval += (int)(cnt / 1.5f);
            cnt %= (int)1.5f;
        }
    } else if (which == 2) {
        auto clock = static_cast<CounterMode::ClockSource2>((mode.clockSource >> 1) & 1);
        using modes = CounterMode::ClockSource2;

        if (clock == modes::systemClock_8) {
            tval += (int)(cnt / (8 * 1.5f));
            cnt %= (int)(8 * 1.5f);
        } else {  // System Clock
            tval += (int)(cnt * 1.5f);
            cnt %= (int)1.5f;
        }
    }

    bool possibleIrq = false;

    if (tval >= target._reg) {
        mode.reachedTarget = true;
        if (mode.resetToZero == CounterMode::ResetToZero::whenTarget) tval = 0;
        if (mode.irqWhenTarget) possibleIrq = true;
    }

    if (tval >= 0xffff) {
        mode.reachedFFFF = true;
        if (mode.resetToZero == CounterMode::ResetToZero::whenFFFF) tval = 0;
        if (mode.irqWhenFFFF) possibleIrq = true;
    }

    if (possibleIrq) checkIrq();

    current._reg = (uint16_t)tval;
}

void Timer::checkIrq() {
    if (mode.irqPulseMode == CounterMode::IrqPulseMode::toggle) {
        mode.interruptRequest = !mode.interruptRequest;
    } else {
        mode.interruptRequest = false;
    }

    if (mode.irqRepeatMode == CounterMode::IrqRepeatMode::oneShot && oneShotIrqOccured) return;

    if (mode.interruptRequest == false) {
        sys->interrupt->trigger(mapIrqNumber());
        oneShotIrqOccured = true;
    }
    mode.interruptRequest = true;  // low only for few cycles
}

uint8_t Timer::read(uint32_t address) {
    if (address < 2) {
        return current.read(address);
    }
    if (address >= 4 && address < 6) {
        uint8_t v = mode.read(address - 4);
        if (address == 5) {
            mode.reachedFFFF = false;
            mode.reachedTarget = false;
        }
        return v;
    }
    if (address >= 8 && address < 10) {
        return target.read(address - 8);
    }
    return 0;
}

void Timer::write(uint32_t address, uint8_t data) {
    if (address < 2) {
        current.write(address, data);
    } else if (address >= 4 && address < 6) {
        current._reg = 0;
        mode.write(address - 4, data);  // BIOS uses 0x0148 for TIMER1

        paused = false;
        if (address == 5) {
            oneShotIrqOccured = false;
            mode.interruptRequest = true;
            if (mode.syncEnabled) {
                if (which == 0) {
                    using modes = CounterMode::SyncMode0;
                    auto mode0 = static_cast<CounterMode::SyncMode0>(mode.syncMode);
                    if (mode0 == modes::pauseUntilHblankAndFreerun) paused = true;

                    fmt::print("[Timer{}]: Synchronization enabled: {}\n", which, (int)mode0);
                }
                if (which == 1) {
                    using modes = CounterMode::SyncMode1;
                    auto mode1 = static_cast<CounterMode::SyncMode1>(mode.syncMode);
                    if (mode1 == modes::pauseUntilVblankAndFreerun) paused = true;

                    fmt::print("[Timer{}]: Synchronization enabled: {}\n", which, (int)mode1);
                }
                if (which == 2) {
                    using modes = CounterMode::SyncMode2;
                    auto mode2 = static_cast<CounterMode::SyncMode2>(mode.syncMode);
                    if (mode2 == modes::stopCounter || mode2 == modes::stopCounter_) paused = true;

                    fmt::print("[Timer{}]: Synchronization enabled: {}\n", which, (int)mode2);
                }
            }
        }
    } else if (address >= 8 && address < 10) {
        target.write(address - 8, data);
    }
}

};  // namespace device::timer