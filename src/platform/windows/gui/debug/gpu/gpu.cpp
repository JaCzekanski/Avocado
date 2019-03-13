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

    ImGui::Text("Display:");
    ImGui::Text("Resolution %dx%d%c @ %dHz", horRes, verRes, interlaced ? 'i' : 'p', mode);
    ImGui::Text("Color depth: %dbit", colorDepth);
    ImGui::Text("areaStart:  %4d:%4d", gpu->displayAreaStartX, gpu->displayAreaStartY);
    ImGui::Text("rangeX:     %4d:%4d", gpu->displayRangeX1, gpu->displayRangeX2);
    ImGui::Text("rangeY:     %4d:%4d", gpu->displayRangeY1, gpu->displayRangeY2);

    ImGui::Text("");
    ImGui::Text("Drawing:");
    ImGui::Text("areaMin:    %4d:%4d", gpu->drawingArea.left, gpu->drawingArea.top);
    ImGui::Text("areaMax:    %4d:%4d", gpu->drawingArea.right, gpu->drawingArea.bottom);
    ImGui::Text("offset:     %4d:%4d", gpu->drawingOffsetX, gpu->drawingOffsetY);
    // ImGui::Text("")

    ImGui::End();
}