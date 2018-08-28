#include "gpu.h"
#include <imgui.h>

bool showGpuWindow = false;

void gpuWindow(System* sys) {
    auto& gpu = sys->gpu;

    ImGui::Begin("GPU", &showGpuWindow, ImVec2(200, 100));

    int horRes = gpu->gp1_08.getHorizontalResoulution();
    int verRes = gpu->gp1_08.getVerticalResoulution();
    bool interlaced = gpu->gp1_08.interlace;
    int mode = gpu->gp1_08.videoMode == gpu::GP1_08::VideoMode::ntsc ? 60 : 50;
    int colorDepth = gpu->gp1_08.colorDepth == gpu::GP1_08::ColorDepth::bit24 ? 24 : 15;

    ImGui::Text("Resolution %dx%d%c @ %dHz", horRes, verRes, interlaced ? 'i' : 'p', mode);
    ImGui::Text("Color depth: %dbit", colorDepth);

    ImGui::End();
}