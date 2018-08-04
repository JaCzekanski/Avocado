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

    ImGui::PlotLines("Preview", samples.data(), samples.size(), 0, nullptr, -1.0f, 1.0f, ImVec2(400, 80));
}

void spuWindow(SPU *spu) {
    if (!showSpuWindow) {
        return;
    }
    static bool parseValues = true;

    const int COL_NUM = 9;
    float columnsWidth[COL_NUM] = {0};
    
    auto column = [&](const std::string& str){
        static int n = 0;
        ImVec2 size = ImGui::CalcTextSize(str.c_str());
        size.x += 20.f;
        if (size.x > columnsWidth[n]) columnsWidth[n] = size.x;
        ImGui::Text(str.c_str());
        ImGui::NextColumn();
        if (++n >= COL_NUM) n = 0;
    };

    auto mapState = [](SPU::Voice::State state) -> std::string {
        switch(state) {
            case SPU::Voice::State::Attack:  return "A   ";
            case SPU::Voice::State::Decay:   return " D  ";
            case SPU::Voice::State::Sustain: return "  S ";
            case SPU::Voice::State::Release: return "   R";
            case SPU::Voice::State::Off:     return "";
        }
    };

    ImGui::Begin("SPU", &showSpuWindow);

    ImGui::Columns(COL_NUM, nullptr, false);

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

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
        auto &v = spu->voices[i];

        if (v.volume.left == 0 && v.volume.right == 0)
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.4f, 0.4f, 1.f));
        else
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 1.f, 1.f, 1.f));

        column(string_format("%d", i + 1));

        column(mapState(v.state));
        if (parseValues) {
            column(string_format("%.0f", v.volume.getLeft()*100.f));
            column(string_format("%.0f", v.volume.getRight()*100.f));
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
        column(string_format("%s %s", 
            spu->voiceChannelReverbMode.getBit(i)?"Reverb":"",
            v.mode == SPU::Voice::Mode::Noise?"Noise":"",
            v.pitchModulation ? "PitchModulation" : ""
        ));
        
        ImGui::PopStyleColor();
    }

    for (int c = 0; c<COL_NUM; c++) {
        ImGui::SetColumnWidth(c, columnsWidth[c]);
    }

    ImGui::Columns(1);
    ImGui::Separator();

    ImGui::Text("Main   volume: %08x", spu->mainVolume._reg);
    ImGui::Text("CD     volume: %08x", spu->cdVolume._reg);
    ImGui::Text("Ext    volume: %08x", spu->extVolume._reg);
    ImGui::Text("Reverb volume: %08x", spu->reverbVolume._reg);
    ImGui::Text("Control:     %04x     %-10s   %-4s   %-8s   %-3s", spu->control._reg, spu->control.spuEnable ? "SPU enable" : "",
                spu->control.mute ? "" : "Mute", spu->control.cdEnable ? "Audio CD" : "", spu->control.irqEnable ? "IRQ" : "");

    ImGui::Text("IRQ Address: %08x", spu->irqAddress._reg*8);

    ImGui::Checkbox("Parse values", &parseValues);

    ImGui::PopStyleVar();

    renderSamples(spu);
    ImGui::End();
}
