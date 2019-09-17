#include "about.h"
#include <imgui.h>
#include "version.h"

namespace gui::help {

void About::aboutWindow() {
    ImGui::Begin("About", &aboutWindowOpen, ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::Text("Avocado %s", BUILD_STRING);
    ImGui::Text("Build date: %s", BUILD_DATE);
    ImGui::End();
}

void About::displayWindows() {
    if (aboutWindowOpen) aboutWindow();
}

}  // namespace gui::help