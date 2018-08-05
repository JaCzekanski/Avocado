#include <imgui.h>
#include <vector>
#include "device/spu/spu.h"
#include "gui.h"
#include "utils/string.h"

using namespace spu;
extern bool showSpuWindow;

void channelsInfo(spu::SPU* spu, bool parseValues) {
    const int COL_NUM = 9;
    float columnsWidth[COL_NUM] = {0};

    auto column = [&](const std::string& str) {
        static int n = 0;
        ImVec2 size = ImGui::CalcTextSize(str.c_str());
        size.x += 16.f;
        if (size.x > columnsWidth[n]) columnsWidth[n] = size.x;
        ImGui::TextUnformatted(str.c_str());
        ImGui::NextColumn();
        if (++n >= COL_NUM) n = 0;
    };

    auto mapState = [](Voice::State state) -> std::string {
        switch (state) {
            case Voice::State::Attack: return "A   ";
            case Voice::State::Decay: return " D  ";
            case Voice::State::Sustain: return "  S ";
            case Voice::State::Release: return "   R";
            case Voice::State::Off: return "";
        }
    };

    auto adsrInfo = [](ADSR adsr) -> std::string {
        std::string s;
        s += "Attack:\n";
        s += string_format("Step:      +%d\n", adsr.attackStep + 4);
        s += string_format("Shift:     %d\n", adsr.attackShift);
        s += string_format("Direction: %s\n", "Increase");
        s += string_format("Mode:      %s\n\n", adsr.attackMode ? "Exponential" : "Linear");

        s += "Decay:\n";
        s += string_format("Step:      -8\n");
        s += string_format("Shift:     %d\n", adsr.decayShift);
        s += string_format("Direction: %s\n", "Decrease");
        s += string_format("Mode:      %s\n\n", "Exponential");

        s += "Sustain:\n";
        s += string_format("Step:      %c%d\n", adsr.sustainDirection ? '-' : '+', adsr.sustainStep);  // TOOD: off by one
        s += string_format("Shift:     %d\n", adsr.sustainStep);
        s += string_format("Direction: %s\n", adsr.sustainDirection ? "Decrease" : "Increase");
        s += string_format("Mode:      %s\n", adsr.sustainMode ? "Exponential" : "Linear");
        s += string_format("Level:     %d\n\n", adsr.sustainLevel);

        s += "Release:\n";
        s += string_format("Step:      -8\n");
        s += string_format("Shift:     %d\n", adsr.releaseShift);
        s += string_format("Direction: %s\n", "Decrease");
        s += string_format("Mode:      %s\n\n", adsr.releaseMode ? "Exponential" : "Linear");

        return s;
    };

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
    ImGui::Columns(COL_NUM, nullptr, false);

    column("");
    column("State");
    column("VolL");
    column("VolR");
    column("Sample rate");
    column("Start addr");
    column("Repeat addr");
    column("ADSR");
    column("Flags");
    for (int i = 0; i < SPU::VOICE_COUNT; i++) {
        auto& v = spu->voices[i];

        if (v.volume.left == 0 && v.volume.right == 0) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.4f, 0.4f, 1.f));
        } else {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 1.f, 1.f, 1.f));
        }

        column(string_format("%d", i + 1));

        column(mapState(v.state));
        if (parseValues) {
            column(string_format("%.0f", v.volume.getLeft() * 100.f));
            column(string_format("%.0f", v.volume.getRight() * 100.f));
        } else {
            column(string_format("%04x", v.volume.left));
            column(string_format("%04x", v.volume.right));
        }

        if (parseValues) {
            column(string_format("%5d Hz", static_cast<int>(std::min((uint16_t)0x1000, v.sampleRate._reg) / 4096.f * 44100.f)));
        } else {
            column(string_format("%04x", v.sampleRate._reg));
        }
        column(string_format("%04x", v.startAddress._reg));
        column(string_format("%04x", v.repeatAddress._reg));
        column(string_format("%08x", v.ADSR._reg));
        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::TextUnformatted(adsrInfo(v.ADSR).c_str());
            ImGui::EndTooltip();
        }

        column(string_format("%s %s", v.reverb ? "Reverb" : "", v.mode == Voice::Mode::Noise ? "Noise" : "",
                             v.pitchModulation ? "PitchModulation" : ""));

        ImGui::PopStyleColor();
    }

    for (int c = 0; c < COL_NUM; c++) {
        ImGui::SetColumnWidth(c, columnsWidth[c]);
    }

    ImGui::Columns(1);
    ImGui::PopStyleVar();
    ImGui::TreePop();
}

void reverbInfo(spu::SPU* spu) {
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
    ImGui::Text("Base: 0x%08x", spu->reverbBase._reg * 8);

    ImGui::Columns(8, nullptr, false);
    for (int y = 0; y < 4; y++) {
        for (int x = 0; x < 8; x++) {
            auto str = string_format("%04x", spu->reverbRegisters[y * 8 + x]);
            ImGui::TextUnformatted(str.c_str());

            ImVec2 size = ImGui::CalcTextSize(str.c_str());
            ImGui::SetColumnWidth(x, size.x + 8.f);

            ImGui::NextColumn();
        }
    }

    ImGui::Columns(1);
    ImGui::PopStyleVar();
    ImGui::TreePop();
}

void registersInfo(spu::SPU* spu) {
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
    ImGui::Text("Main   volume: %08x", spu->mainVolume._reg);
    ImGui::Text("CD     volume: %08x", spu->cdVolume._reg);
    ImGui::Text("Ext    volume: %08x", spu->extVolume._reg);
    ImGui::Text("Reverb volume: %08x", spu->reverbVolume._reg);
    ImGui::Text("Control: 0x%04x     %-10s   %-4s   %-8s   %-3s", spu->control._reg, spu->control.spuEnable ? "SPU enable" : "",
                spu->control.mute ? "" : "Mute", spu->control.cdEnable ? "Audio CD" : "", spu->control.irqEnable ? "IRQ" : "");

    ImGui::Text("Status:  0x%0x04  %-3s",  spu->SPUSTAT._reg,
        (spu->SPUSTAT._reg & (1<<6))?"IRQ":"");

    ImGui::Text("IRQ Address: 0x%08x", spu->irqAddress._reg * 8);
    ImGui::PopStyleVar();
    ImGui::TreePop();
}

void renderSamples(spu::SPU* spu) {
    std::vector<float> samples;
    for (auto s : spu->audioBuffer) {
        samples.push_back((float)s / (float)0x7fff);
    }

    ImGui::PlotLines("Preview", samples.data(), samples.size(), 0, nullptr, -1.0f, 1.0f, ImVec2(400, 80));
}

void spuWindow(spu::SPU* spu) {
    if (!showSpuWindow) {
        return;
    }
    const auto treeFlags = ImGuiTreeNodeFlags_CollapsingHeader | ImGuiTreeNodeFlags_DefaultOpen;
    static bool parseValues = true;

    ImGui::Begin("SPU", &showSpuWindow);

    if (ImGui::TreeNodeEx("Channels", treeFlags)) channelsInfo(spu, parseValues);
    if (ImGui::TreeNodeEx("Reverb", treeFlags)) reverbInfo(spu);
    if (ImGui::TreeNodeEx("Registers", treeFlags)) registersInfo(spu);

    ImGui::Checkbox("Parse values", &parseValues);
    renderSamples(spu);
    ImGui::End();
}
