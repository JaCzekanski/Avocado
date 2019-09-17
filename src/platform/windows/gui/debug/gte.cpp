#include "gte.h"
#include <fmt/core.h>
#include <imgui.h>
#include <numeric>
#include "config.h"
#include "system.h"

namespace gui::debug {

void GTE::logWindow(System* sys) {
    static char filterBuffer[16];
    static bool searchActive = false;
    bool enabled = config["debug"]["log"]["gte"].get<int>();
    bool filterActive = strlen(filterBuffer) > 0;
    ImGui::Begin("GTE Log", &logWindowOpen, ImVec2(300, 400));

    ImGui::BeginChild("GTE Log", ImVec2(0, -ImGui::GetItemsLineHeightWithSpacing()), false, ImGuiWindowFlags_HorizontalScrollbar);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

    for (size_t i = 0; i < sys->cpu->gte.log.size(); i++) {
        auto ioEntry = sys->cpu->gte.log.at(i);
        std::string t;
        if (ioEntry.mode == ::GTE::GTE_ENTRY::MODE::func) {
            t = fmt::format("{:5d} {} 0x{:02x}", i, 'F', ioEntry.n);
        } else {
            t = fmt::format("{:5d} {} {:2d}: 0x{:08x}", i, ioEntry.mode == ::GTE::GTE_ENTRY::MODE::read ? 'R' : 'W', ioEntry.n,
                            ioEntry.data);
        }
        if (filterActive && t.find(filterBuffer) != std::string::npos)  // if found
        {
            // if search enabled - continue
            ImGui::TextColored(ImVec4(1.f, 1.f, 0.f, 1.f), "%s", t.c_str());
            continue;
        }
        if (searchActive) continue;

        ImGui::Text("%s", t.c_str());
    }
    ImGui::PopStyleVar();
    ImGui::EndChild();

    if (ImGui::Checkbox("Log enabled", &enabled)) {
        config["debug"]["log"]["gte"] = (int)enabled;
        bus.notify(Event::Config::Gte{});
    }
    ImGui::SameLine();

    ImGui::Text("Filter");
    ImGui::SameLine();
    if (ImGui::InputText("", filterBuffer, sizeof(filterBuffer) / sizeof(char), ImGuiInputTextFlags_EnterReturnsTrue)) {
        searchActive = !searchActive;
    }

    ImGui::End();
}

void GTE::registersWindow(System* sys) {
    auto& gte = sys->cpu->gte;
    ImGui::Begin("Gte", &registersWindowOpen, ImVec2(600, 300), ImGuiWindowFlags_NoScrollbar);

    ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[1]);

    ImGui::Columns(3, nullptr, false);
    ImGui::Text("IR1:  %04hX", gte.ir[1]);
    ImGui::NextColumn();
    ImGui::Text("IR2:  %04hX", gte.ir[2]);
    ImGui::NextColumn();
    ImGui::Text("IR3:  %04hX", gte.ir[3]);
    ImGui::NextColumn();

    ImGui::Separator();
    ImGui::Text("MAC0: %08X", gte.mac[0]);

    ImGui::Separator();
    ImGui::Text("MAC1: %08X", gte.mac[1]);
    ImGui::NextColumn();
    ImGui::Text("MAC2: %08X", gte.mac[2]);
    ImGui::NextColumn();
    ImGui::Text("MAC3: %08X", gte.mac[3]);
    ImGui::NextColumn();

    ImGui::Separator();
    ImGui::Text("TRX:  %08X", gte.translation.x);
    ImGui::NextColumn();
    ImGui::Text("TRY:  %08X", gte.translation.y);
    ImGui::NextColumn();
    ImGui::Text("TRZ:  %08X", gte.translation.z);
    ImGui::NextColumn();

    ImGui::Separator();

    for (int i = 0; i < 4; i++) {
        ImGui::Text("S%dX:  %04hX", i, gte.s[i].x);
        ImGui::NextColumn();
        ImGui::Text("S%dY:  %04hX", i, gte.s[i].y);
        ImGui::NextColumn();
        ImGui::Text("S%dZ:  %04hX", i, gte.s[i].z);
        ImGui::NextColumn();
    }

    ImGui::Separator();

    for (int y = 0; y < 3; y++) {
        for (int x = 0; x < 3; x++) {
            ImGui::Text("RT%d%d:  %04hX", y + 1, x + 1, gte.rotation[y][x]);
            ImGui::NextColumn();
        }
    }

    ImGui::Separator();
    ImGui::Text("VX0:  %04hX", gte.v[0].x);
    ImGui::NextColumn();
    ImGui::Text("VY0:  %04hX", gte.v[0].y);
    ImGui::NextColumn();
    ImGui::Text("VZ0:  %04hX", gte.v[0].z);
    ImGui::NextColumn();

    ImGui::Separator();
    ImGui::Text("OFX:  %08X", gte.of[0]);
    ImGui::NextColumn();
    ImGui::Text("OFY:  %08X", gte.of[1]);
    ImGui::NextColumn();
    ImGui::Text("H:   %04hX", gte.h);
    ImGui::NextColumn();

    ImGui::Separator();
    ImGui::Text("DQA:  %04hX", gte.dqa);
    ImGui::NextColumn();
    ImGui::Text("DQB:  %08X", gte.dqb);
    ImGui::NextColumn();

    ImGui::PopFont();

    ImGui::End();
}

void GTE::displayWindows(System* sys) {
    if (logWindowOpen) logWindow(sys);
    if (registersWindowOpen) registersWindow(sys);
}
}  // namespace gui::debug