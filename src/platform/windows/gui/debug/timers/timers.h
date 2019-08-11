#pragma once
#include <array>
#include <cstdint>
#include <string>

struct System;

namespace gui::debug::timers {
class Timers {
    void timersWindow(System* sys);

   public:
    bool timersWindowOpen = false;
    void displayWindows(System* sys);
};
}  // namespace gui::debug::timers