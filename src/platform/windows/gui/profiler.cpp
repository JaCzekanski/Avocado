#include "profiler.h"
#include <imgui.h>
#include <SDL.h>
#include "utils/profiler/profiler.h"

bool showProfilerWindow = false;

uint64_t getExclusiveTime(uint64_t start, uint64_t end) {
    uint64_t biggestTime = 0;
    for (auto& group : Profiler::profiles) {
        for (auto& sample : group.second.samples) {
            if (sample.start > start && sample.end < end && sample.end - sample.start > biggestTime) {
                biggestTime = sample.end - sample.start;
            }
        }
    }

    return biggestTime;
}

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

    ImGui::Text("Inclusive time, exclusive time, calls");
    for (auto& group : Profiler::profiles) {
        double exclusive = 0.0;  // only this function EXCLUDING inner
        double inclusive = 0.0;  // whole function

        for (auto& sample : group.second.samples) {
            inclusive += sample.end - sample.start;
            //            exclusive += sample.end - sample.start - getExclusiveTime(sample.start, sample.end);
        }

        double inclusiveTime = inclusive * 1000.0 / SDL_GetPerformanceFrequency();
        double exclusiveTime = exclusive * 1000.0 / SDL_GetPerformanceFrequency();
        int calls = group.second.samples.size();
        if (calls > 1) {
            ImGui::Text("%s: %7.4f ms, %7.4f ms, calls: %d", group.first, inclusiveTime, exclusiveTime, calls);
        } else {
            ImGui::Text("%s: %7.4f ms, %7.4f ms", group.first, inclusiveTime, exclusiveTime);
        }

        // exclusive - find profile with earliest begin time (in relative to current sample)
        // and with longest end time
    }
    uint64_t totalTime = Profiler::getTotalTime();
    ImGui::Text("%s: %7.4f ms", "Total", totalTime * 1000.0 / SDL_GetPerformanceFrequency());

    ImGui::End();
}