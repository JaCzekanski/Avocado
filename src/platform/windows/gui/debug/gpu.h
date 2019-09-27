#pragma once
#include <imgui.h>
#include <memory>
#include <string>
#include <vector>

struct System;
class Texture;

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
    int busToken = -1;

    std::vector<Area> vramAreas;
    float blinkTimer = 0.f;

    std::unique_ptr<Texture> textureImage;
    std::vector<uint8_t> textureUnpacked;
    std::unique_ptr<Texture> vramImage;
    std::vector<uint8_t> vramUnpacked;

    void registersWindow(System *sys);
    void logWindow(System *sys);
    void vramWindow(gpu::GPU *gpu);

   public:
    bool registersWindowOpen = false;
    bool logWindowOpen = false;
    bool vramWindowOpen = false;

    GPU();
    ~GPU();
    void displayWindows(System *sys);
};
}  // namespace gui::debug