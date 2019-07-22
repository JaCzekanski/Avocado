#pragma once
#include <array>
#include <cstdint>
#include <string>
#include "system.h"

struct System;

namespace gui::debug::cpu {
struct Segment {
    const char* name;
    uint32_t base;
    uint32_t size;

    bool inRange(uint32_t addr) { return addr >= base && addr < base + size; }

    static Segment fromAddress(uint32_t);
};

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