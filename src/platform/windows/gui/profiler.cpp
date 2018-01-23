#include "profiler.h"
#include <imgui.h>
#include <SDL.h>
#include "utils/profiler/profiler.h"

bool showProfilerWindow = false;

void profilerWindow() {
    ImGui::Begin("Profiler", &showProfilerWindow, ImVec2(300, 200));

    bool profilerEnabled = Profiler::isEnabled();
    if (ImGui::Checkbox("Enabled", &profilerEnabled)) {
        if (profilerEnabled)
            Profiler::enable();
        else {
            Profiler::disable();
            Profiler::clear();
        }
    }

    for (auto& group : Profiler::profiles) {
        double time = group.second.total() * 1000.0 / SDL_GetPerformanceFrequency();
        int calls = group.second.samples.size();
        if (calls > 1) {
            ImGui::Text("%s: %7.4f ms, calls: %d", group.first, time, calls);
        } else {
            ImGui::Text("%s: %7.4f ms", group.first, time);
        }
    }
    uint64_t totalTime = Profiler::getTotalTime();
    ImGui::Text("%s: %7.4f ms", "Total", totalTime * 1000.0 / SDL_GetPerformanceFrequency());

    ImGui::End();
}