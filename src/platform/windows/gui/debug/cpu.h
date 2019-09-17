#pragma once
#include <cstdint>

struct System;

namespace gui::debug {

class CPU {
    char addrInputBuffer[32];
    uint32_t contextMenuAddress = 0;
    uint32_t goToAddr = 0;
    uint32_t prevPC = 0;

    void debuggerWindow(System* sys);
    void breakpointsWindow(System* sys);

   public:
    bool debuggerWindowOpen = false;
    bool breakpointsWindowOpen = false;
    void displayWindows(System* sys);
};
}  // namespace gui::debug::cpu