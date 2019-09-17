#pragma once
#include <imgui.h>
#include <cstdint>
#include <string>
#include <vector>
#include "imgui/imgui_memory_editor.h"

struct System;

namespace gui::debug {

class CPU {
    struct Watch {
        uint32_t address;
        int size;  // 1, 2, 4
        std::string name;
    };
    std::vector<Watch> watches = {
        {0x1F801070, 2, "ISTAT"},
        {0x1F801074, 2, "IMASK"},
    };

    char addrInputBuffer[32];
    uint32_t contextMenuAddress = 0;
    uint32_t goToAddr = 0;
    uint32_t prevPC = 0;

    MemoryEditor editor;

    void debuggerWindow(System* sys);
    void breakpointsWindow(System* sys);
    void watchWindow(System* sys);
    void ramWindow(System* sys);

   public:
    bool debuggerWindowOpen = false;
    bool breakpointsWindowOpen = false;
    bool watchWindowOpen = false;
    bool ramWindowOpen = false;

    void displayWindows(System* sys);
};
}  // namespace gui::debug