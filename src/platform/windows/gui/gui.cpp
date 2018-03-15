#include "gui.h"
#include <imgui.h>
#include "cpu/gte/gte.h"
#include "imgui/imgui_impl_sdl_gl3.h"
#include "options.h"
#include "platform/windows/config.h"
#include "version.h"

void openFileWindow();

void gteRegistersWindow(GTE gte);
void ioLogWindow(System* sys);
void gteLogWindow(System* sys);
void gpuLogWindow(System* sys);
void ioWindow(System* sys);
void vramWindow();
void disassemblyWindow(System* sys);
void breakpointsWindow(System* sys);
void watchWindow(System* sys);
void ramWindow(System* sys);
void cdromWindow(System* sys);

bool showGui = true;

int vramTextureId = 0;
bool gteRegistersEnabled = false;
bool ioLogEnabled = false;
bool gteLogEnabled = false;
bool gpuLogEnabled = false;
bool showVRAM = false;
bool singleFrame = false;
bool showIo = false;
bool exitProgram = false;
bool doHardReset = false;
bool waitingForKeyPress = false;
SDL_Keycode lastPressedKey = 0;

bool notInitializedWindowShown = false;
bool showVramWindow = false;
bool showDisassemblyWindow = false;
bool showBreakpointsWindow = false;
bool showWatchWindow = false;
bool showRamWindow = false;
bool showCdromWindow = false;

bool showAboutWindow = false;

void aboutWindow() {
    ImGui::Begin("About", &showAboutWindow);
    ImGui::Text("Avocado %s", BUILD_STRING);
    ImGui::Text("Build date: %s", BUILD_DATE);
    ImGui::End();
}

void renderImgui(System* sys) {
    if (showGui) {
        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("Exit", "Esc")) exitProgram = true;
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Emulation")) {
                if (ImGui::MenuItem("Soft reset", "F2")) sys->softReset();
                if (ImGui::MenuItem("Hard reset")) doHardReset = true;

                const char* shellStatus = sys->cdrom->getShell() ? "Shell opened" : "Shell closed";
                if (ImGui::MenuItem(shellStatus, "F3")) {
                    sys->cdrom->toggleShell();
                }

                if (ImGui::MenuItem("Single frame", "F7")) {
                    singleFrame = true;
                    sys->state = System::State::run;
                }

                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Debug")) {
                ImGui::MenuItem("BIOS log", nullptr, &sys->biosLog);
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
                ImGui::MenuItem("Watch", nullptr, &showWatchWindow);
                ImGui::MenuItem("CDROM", nullptr, &showCdromWindow);
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Options")) {
                if (ImGui::MenuItem("BIOS", nullptr)) showBiosWindow = true;
                if (ImGui::MenuItem("Controller", nullptr)) showControllerSetupWindow = true;
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Help")) {
                if (ImGui::MenuItem("About")) showAboutWindow = true;
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }

        // File

        // Debug
        if (gteRegistersEnabled) gteRegistersWindow(sys->cpu->gte);
        if (ioLogEnabled) ioLogWindow(sys);
        if (gteLogEnabled) gteLogWindow(sys);
        if (gpuLogEnabled) gpuLogWindow(sys);
        if (showIo) ioWindow(sys);
        if (showRamWindow) ramWindow(sys);
        if (showVramWindow) vramWindow();
        if (showDisassemblyWindow) disassemblyWindow(sys);
        if (showBreakpointsWindow) breakpointsWindow(sys);
        if (showWatchWindow) watchWindow(sys);
        if (showCdromWindow) cdromWindow(sys);

        // Options
        if (showBiosWindow) biosSelectionWindow();
        if (showControllerSetupWindow) controllerSetupWindow();

        // Help
        if (showAboutWindow) aboutWindow();
    }

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
    ImGui_ImplSdlGL3_RenderDrawData(ImGui::GetDrawData());
}
