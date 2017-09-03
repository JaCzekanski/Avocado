#include "gui.h"
#include <imgui.h>
#include <filesystem>
#include "gte.h"
#include "platform/windows/config.h"

using namespace std::experimental::filesystem::v1;

void gteRegistersWindow(mips::gte::GTE gte);
void ioLogWindow(mips::CPU* cpu);
void gteLogWindow(mips::CPU* cpu);
void gpuLogWindow(mips::CPU* cpu);
void ioWindow(mips::CPU* cpu);
void vramWindow();
void disassemblyWindow(mips::CPU* cpu);
void biosSelectionWindow();
void controllerSetupWindow();

int vramTextureId = 0;
bool gteRegistersEnabled = false;
bool ioLogEnabled = false;
bool gteLogEnabled = false;
bool gpuLogEnabled = false;
bool showVRAM = false;
bool singleFrame = false;
bool skipRender = false;
bool showIo = false;
bool exitProgram = false;
bool doHardReset = false;
bool waitingForKeyPress = false;
SDL_Keycode lastPressedKey = 0;

bool notInitializedWindowShown = false;
bool showVramWindow = false;
bool showBiosWindow = false;
bool showControllerSetupWindow = false;
bool showDisassemblyWindow = false;

void renderImgui(mips::CPU* cpu) {
    auto gte = cpu->gte;

    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Exit")) exitProgram = true;
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Emulation")) {
            if (ImGui::MenuItem("Soft reset", "F2")) cpu->softReset();
            if (ImGui::MenuItem("Hard reset")) doHardReset = true;

            const char* shellStatus = cpu->cdrom->getShell() ? "Shell opened" : "Shell closed";
            if (ImGui::MenuItem(shellStatus, "F3")) {
                cpu->cdrom->toggleShell();
            }

            if (ImGui::MenuItem("Single frame", "F7")) {
                singleFrame = true;
                cpu->state = mips::CPU::State::run;
            }

            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Debug")) {
            ImGui::MenuItem("IO", nullptr, &showIo);
            ImGui::MenuItem("GTE registers", nullptr, &gteRegistersEnabled);
            ImGui::MenuItem("BIOS log", "F5", &cpu->biosLog);
#ifdef ENABLE_IO_LOG
            ImGui::MenuItem("IO log", nullptr, &ioLogEnabled);
#endif
            ImGui::MenuItem("GTE log", nullptr, &gteLogEnabled);
            ImGui::MenuItem("GPU log", nullptr, &gpuLogEnabled);
            ImGui::MenuItem("VRAM window", nullptr, &showVramWindow);
            ImGui::MenuItem("Disassembly", "F6", &showDisassemblyWindow);
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Options")) {
            if (ImGui::MenuItem("BIOS", nullptr)) showBiosWindow = true;
            if (ImGui::MenuItem("Controller", nullptr)) showControllerSetupWindow = true;
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }

    gteRegistersWindow(gte);
    ioLogWindow(cpu);
    gteLogWindow(cpu);
    gpuLogWindow(cpu);
    ioWindow(cpu);
    if (showVramWindow) vramWindow();
    if (showDisassemblyWindow) disassemblyWindow(cpu);

    if (showBiosWindow) biosSelectionWindow();
    if (showControllerSetupWindow) controllerSetupWindow();

    if (!config["initialized"] && !notInitializedWindowShown) {
        notInitializedWindowShown = true;
        ImGui::OpenPopup("Avocado");
    }

    if (ImGui::BeginPopupModal("Avocado", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Avocado needs to be set up before running.");
        ImGui::Text("You need one of BIOS files placed in data/bios directory.");

        if (ImGui::Button("Select BIOS file")) {
            showBiosWindow = true;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }

    ImGui::Render();
}
