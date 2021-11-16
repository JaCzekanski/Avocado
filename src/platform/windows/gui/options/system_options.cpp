#include "system_options.h"
#include "config.h"
#include <imgui.h>

namespace gui::options {
void System::displayWindows() {
    if (!systemWindowOpen) return;

    ImGui::Begin("System", &systemWindowOpen, ImGuiWindowFlags_AlwaysAutoResize);

    if (ImGui::Checkbox("8MB ram", &config.options.system.ram8mb)) {
        bus.notify(Event::System::HardReset{});
    }

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.8f, 0.8f, 1.f));
    ImGui::Text(
        "Warning: Changing any of these settings\n"
        "         will result in system hard reset.");
    ImGui::PopStyleColor();

    ImGui::End();
}
};  // namespace gui::options