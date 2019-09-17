#pragma once
#include <cstdint>

struct System;

namespace gui::debug {

class IO {
    void logWindow(System* sys);

   public:
    bool logWindowOpen = false;

    void displayWindows(System* sys);
};
}  // namespace gui::debug