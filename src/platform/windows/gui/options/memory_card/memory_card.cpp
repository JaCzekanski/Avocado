#include "memory_card.h"
#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>
#include "config.h"
#include "system.h"
#include "utils/string.h"

namespace gui::options::memory_card {
void MemoryCard::memoryCardWindow(System* sys) {
    if (loadPaths) {
        for (size_t i = 0; i < cardPaths.size(); i++) {
            cardPaths[i] = config["memoryCard"][std::to_string(i + 1)];
        }
        loadPaths = false;
    }
    ImGui::Begin("MemoryCard", &memoryCardWindowOpen, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse);
    ImGui::BeginTabBar("##memory_select");

    for (size_t i = 0; i < sys->controller->card.size(); i++) {
        if (ImGui::BeginTabItem(string_format("Slot %d", i + 1).c_str())) {
            if (ImGui::InputText("Path", &cardPaths[i])) {
                config["memoryCard"][std::to_string(i + 1)] = cardPaths[i];
            }
            // TODO: Reload contents on change?
            // TODO: Check if card exists, override or reload?

            bool inserted = sys->controller->card[i]->inserted;
            if (ImGui::Checkbox("Inserted", &inserted)) {
                sys->controller->card[0]->inserted = inserted;
            }

            ImGui::EndTabItem();
        }
    }

    ImGui::EndTabBar();
    ImGui::End();
}

void MemoryCard::displayWindows(System* sys) {
    if (memoryCardWindowOpen) memoryCardWindow(sys);
}
}  // namespace gui::options::memory_card