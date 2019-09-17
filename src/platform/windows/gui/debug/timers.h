#pragma once
#include <string>

struct System;
namespace device::timer {
class Timer;
};

namespace gui::debug {
class Timers {
    std::string getSyncMode(device::timer::Timer* timer);
    std::string getClockSource(device::timer::Timer* timer);
    void timersWindow(System* sys);

   public:
    bool timersWindowOpen = false;
    void displayWindows(System* sys);
};
}  // namespace gui::debug