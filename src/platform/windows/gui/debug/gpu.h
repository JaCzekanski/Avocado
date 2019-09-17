#pragma once
#include <imgui.h>
#include <string>
#include <vector>

struct System;

namespace gpu {
class GPU;
};

namespace gui::debug {
class GPU {
    struct Area {
        std::string name;
        ImVec2 pos;
        ImVec2 size;
    };
    std::vector<Area> vramAreas;
    float blinkTimer = 0.f;

    void registersWindow(System *sys);
    void logWindow(System *sys);
    void vramWindow(gpu::GPU *gpu);

   public:
    bool registersWindowOpen = false;
    bool logWindowOpen = false;
    bool vramWindowOpen = false;

    void displayWindows(System *sys);
};
}  // namespace gui::debug