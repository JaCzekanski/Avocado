#pragma once
#include <imgui.h>
#include <memory>
#include <string>
#include <vector>
#include <system.h>

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

    // GPU Log
    gpu::GP0_E1 last_e1;
    int16_t last_offset_x;
    int16_t last_offset_y;
    void printCommandDetails(const gpu::LogEntry &entry);
    void handlePolygonCommand(const gpu::PolygonArgs arg, const std::vector<uint32_t> &arguments);
    void handleLineCommand(const gpu::LineArgs arg, const std::vector<uint32_t> &arguments);
    void handleRectangleCommand(const gpu::RectangleArgs arg, const std::vector<uint32_t> &arguments);

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