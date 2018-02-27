#include <imgui.h>
#include <json.hpp>
#include <vector>
#include "debugger/debugger.h"
#include "gui.h"
#include "utils/string.h"

extern bool showCdromWindow;

#define POSITION(x) ((useLba) ? string_format("%d", (x).toLba()).c_str() : (x).toString().c_str())

void cdromWindow(mips::CPU *cpu) {
    if (!showCdromWindow) {
        return;
    }
    static bool useLba = false;

    ImGui::Begin("CDROM", &showCdromWindow, ImVec2(300, 400));

    auto cue = cpu->cdrom->cue;

    if (cue.file.empty()) {
        ImGui::Text("No CD");
    } else {
        ImGui::Text("%s", cue.file.c_str());

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
        ImGui::Text("Track  Pregap    Pause     Start     End       Offset B   Type   File");
        ImGuiListClipper clipper(cue.getTrackCount());
        while (clipper.Step()) {
            for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++) {
                auto track = cue.tracks[i];

                bool nodeOpened
                    = ImGui::TreeNode((void *)(intptr_t)i, "%02d  %-8s  %-8s  %-8s  %-8s  %-9d  %-5s  %s", i, POSITION(track.pregap),
                                      POSITION(track.pause), POSITION(track.start), POSITION(track.end), track.offsetInFile,
                                      track.type == utils::Track::Type::DATA ? "DATA" : "AUDIO", track.filename.c_str());

                if (nodeOpened) {
                    ImGui::TreePop();
                }
            }
        }
        ImGui::PopStyleVar();

        ImGui::Checkbox("Use LBA", &useLba);
    }
    ImGui::End();
}