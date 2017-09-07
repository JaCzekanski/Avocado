#include "gui.h"
#include <imgui.h>
#include <filesystem>
#include "gte.h"
#include "platform/windows/config.h"

using namespace std::experimental::filesystem::v1;

void openFileWindow();

void gteRegistersWindow(mips::gte::GTE gte);
void ioLogWindow(mips::CPU* cpu);
void gteLogWindow(mips::CPU* cpu);
void gpuLogWindow(mips::CPU* cpu);
void ioWindow(mips::CPU* cpu);
void vramWindow();
void disassemblyWindow(mips::CPU* cpu);
void breakpointsWindow(mips::CPU* cpu);
void ramWindow(mips::CPU* cpu);
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
bool showBreakpointsWindow = false;
bool showRamWindow = false;

void renderImgui(mips::CPU* cpu) {
    auto gte = cpu->gte;

    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Exit", "Esc")) exitProgram = true;
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
            ImGui::MenuItem("BIOS log", nullptr, &cpu->biosLog);
#ifdef ENABLE_IO_LOG
            ImGui::MenuItem("IO log", nullptr, &ioLogEnabled);
#endif
            ImGui::MenuItem("GTE log", nullptr, &gteLogEnabled);
            ImGui::MenuItem("GPU log", nullptr, &gpuLogEnabled);

            ImGui::Separator();

            ImGui::MenuItem("GTE registers", nullptr, &gteRegistersEnabled);
            ImGui::MenuItem("IO", nullptr, &showIo);
            ImGui::MenuItem("Memory", nullptr, &showRamWindow);
            ImGui::MenuItem("Video memory", nullptr, &showVramWindow);
            ImGui::MenuItem("Debugger", nullptr, &showDisassemblyWindow);
            ImGui::MenuItem("Breakpoints", nullptr, &showBreakpointsWindow);
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Options")) {
            if (ImGui::MenuItem("BIOS", nullptr)) showBiosWindow = true;
            if (ImGui::MenuItem("Controller", nullptr)) showControllerSetupWindow = true;
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }

    // File

    // Debug
    gteRegistersWindow(gte);
    ioLogWindow(cpu);
    gteLogWindow(cpu);
    gpuLogWindow(cpu);
    ioWindow(cpu);
    if (showRamWindow) ramWindow(cpu);
    if (showVramWindow) vramWindow();
    if (showDisassemblyWindow) disassemblyWindow(cpu);
    if (showBreakpointsWindow) breakpointsWindow(cpu);

    // Options
    if (showBiosWindow) biosSelectionWindow();
    if (showControllerSetupWindow) controllerSetupWindow();

    if (!isEmulatorConfigured() && !notInitializedWindowShown) {
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
