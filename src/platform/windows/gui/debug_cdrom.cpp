#include <imgui.h>
#include <nlohmann/json.hpp>
#include <vector>
#include "gui.h"
#include "system.h"
#include "utils/string.h"

extern bool showCdromWindow;

#define POSITION(x) ((useFrames) ? string_format("%d", (x).toLba()).c_str() : (x).toString().c_str())

void cdromWindow(System* sys) {
    if (!showCdromWindow) {
        return;
    }
    static bool useFrames = false;

    ImGui::Begin("CDROM", &showCdromWindow);

    // auto cue = sys->cdrom->cue;

    // if (cue.file.empty()) {
    //     ImGui::Text("No CD");
    // } else {
    //     ImGui::Text("%s", cue.file.c_str());

    //     ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
    //     ImGui::Text("Track  Pregap     Start     End       Offset   Type   File");

    //     ImGuiListClipper clipper((int)cue.getTrackCount());
    //     while (clipper.Step()) {
    //         for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++) {
    //             auto track = cue.tracks[i];

    //             bool nodeOpened
    //                 = ImGui::TreeNode((void*)(intptr_t)i, "%02d  %-8s  %-8s  %-8s  %-9zu  %-5s  %s", i, POSITION(track.pregap),
    //                                   POSITION(track.index1), POSITION(track.index1 + Position::fromLba(track.frames)), track.offset,
    //                                   track.type == utils::Track::Type::DATA ? "DATA" : "AUDIO", getFilenameExt(track.filename).c_str());

    //             if (nodeOpened) {
    //                 ImGui::TreePop();
    //             }
    //         }
    //     }
    //     ImGui::PopStyleVar();

    //     ImGui::Checkbox("Use frames", &useFrames);
    // }
    ImGui::End();
}