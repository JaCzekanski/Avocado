#include <imgui.h>
#include <vector>
#include "device/spu.h"
#include "gui.h"
#include "utils/string.h"

extern bool showSpuWindow;

void renderSamples(SPU *spu) {
    std::vector<float> samples;
    for (auto s : spu->audioBuffer) {
        samples.push_back((float)s / (float)0x7fff);
    }

    ImGui::PlotLines("Preview", samples.data(), samples.size(), 0, 0, -1.0f, 1.0f, ImVec2(0, 80));
}

void spuWindow(SPU *spu) {
    if (!showSpuWindow) {
        return;
    }
    ImGui::Begin("SPU", &showSpuWindow);

    ImGui::Columns(8, nullptr, false);

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

    ImGui::Text("");
    ImGui::NextColumn();

    ImGui::Text("Flags");
    ImGui::NextColumn();

    ImGui::Text("VolL");
    ImGui::NextColumn();

    ImGui::Text("VolR");
    ImGui::NextColumn();

    ImGui::Text("Sample rate");
    ImGui::NextColumn();

    ImGui::Text("Start addr");
    ImGui::NextColumn();

    ImGui::Text("ADSR");
    ImGui::NextColumn();

    //    ImGui::Text("ADSRVolume");
    //    ImGui::NextColumn();

    ImGui::Text("Repeat addr");
    ImGui::NextColumn();

    for (int i = 0; i < SPU::VOICE_COUNT; i++) {
        auto &v = spu->voices[i];

        if (v.volumeLeft._reg == 0 && v.volumeRight._reg == 0)
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.4f, 0.4f, 1.f));
        else
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 1.f, 1.f, 1.f));

        ImGui::Text("%d", i + 1);
        ImGui::NextColumn();

        bool keyOn = v.playing;
        ImGui::Text("%c", keyOn ? 'P' : ' ');
        ImGui::NextColumn();

        ImGui::Text("%04x", v.volumeLeft._reg);
        ImGui::NextColumn();
        ImGui::Text("%04x", v.volumeRight._reg);
        ImGui::NextColumn();
        ImGui::Text("%04x", v.sampleRate._reg);
        ImGui::NextColumn();
        ImGui::Text("%04x", v.startAddress._reg);
        ImGui::NextColumn();
        ImGui::Text("%08x", v.ADSR._reg);
        ImGui::NextColumn();
        //        ImGui::Text("%04x", v.ADSRVolume._reg);
        //        ImGui::NextColumn();
        ImGui::Text("%04x", v.repeatAddress._reg);
        ImGui::NextColumn();

        ImGui::PopStyleColor();
    }

    ImGui::Columns(1);
    ImGui::Separator();

    ImGui::Text("Main volume: %08x", spu->mainVolume._reg);
    ImGui::Text("CD   volume: %08x", spu->cdVolume._reg);
    ImGui::Text("Ext  volume: %08x", spu->extVolume._reg);
    ImGui::Text("Control:     %04x     %-10s   %-4s   %-8s   %-3s", spu->control._reg, spu->control.spuEnable ? "SPU enable" : "",
                spu->control.mute ? "" : "Mute", spu->control.cdEnable ? "Audio CD" : "", spu->control.irqEnable ? "IRQ" : "");

    ImGui::PopStyleVar();

    renderSamples(spu);
    ImGui::End();
}
