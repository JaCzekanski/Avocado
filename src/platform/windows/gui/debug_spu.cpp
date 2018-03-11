#include <imgui.h>
#include <vector>
#include "device/spu.h"
#include "gui.h"
#include "utils/string.h"

extern bool showSpuWindow;

void spuWindow(SPU *spu) {
    if (!showSpuWindow) {
        return;
    }
    ImGui::Begin("SPU", &showSpuWindow);

    ImGui::Columns(7, nullptr, false);

    ImGui::Text("");
    ImGui::NextColumn();

    ImGui::Text("Volume");
    ImGui::NextColumn();

    ImGui::Text("Sample rate");
    ImGui::NextColumn();

    ImGui::Text("Start address");
    ImGui::NextColumn();

    ImGui::Text("ADSR");
    ImGui::NextColumn();

    ImGui::Text("ADSRVolume");
    ImGui::NextColumn();

    ImGui::Text("Repeat address");
    ImGui::NextColumn();

    for (int i = 0; i < SPU::VOICE_COUNT; i++) {
        auto &v = spu->voices[i];
        ImGui::Text("%d", i + 1);
        ImGui::NextColumn();
        ImGui::Text("%08x", v.volume);
        ImGui::NextColumn();
        ImGui::Text("%04x", v.sampleRate);
        ImGui::NextColumn();
        ImGui::Text("%04x", v.startAddress);
        ImGui::NextColumn();
        ImGui::Text("%08x", v.ADSR);
        ImGui::NextColumn();
        ImGui::Text("%04x", v.ADSRVolume);
        ImGui::NextColumn();
        ImGui::Text("%04x", v.repeatAddress);
        ImGui::NextColumn();
    }

    ImGui::End();
}
