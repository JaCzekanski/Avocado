#include "gui.h"
#include <imgui.h>
#include "config.h"
#include "cpu/gte/gte.h"
#include "debug/cpu/cpu.h"
#include "debug/gpu/gpu.h"
#include "debug/kernel/kernel.h"
#include "debug/timers/timers.h"
#include "imgui/imgui_impl_sdl_gl3.h"
#include "options.h"
#include "platform/windows/input/key.h"
#include "version.h"

void openFileWindow();

void gteRegistersWindow(GTE& gte);
void ioLogWindow(System* sys);
void gteLogWindow(System* sys);
void gpuLogWindow(System* sys);
void vramWindow(gpu::GPU* gpu);
void watchWindow(System* sys);
void ramWindow(System* sys);
void cdromWindow(System* sys);
void spuWindow(spu::SPU* spu);

bool showGui = true;

bool gteRegistersEnabled = false;
bool ioLogEnabled = false;
bool gteLogEnabled = false;
bool gpuLogEnabled = false;
bool singleFrame = false;

bool notInitializedWindowShown = false;
bool showVramWindow = false;
bool showWatchWindow = false;
bool showRamWindow = false;
bool showCdromWindow = false;
bool showSpuWindow = false;

bool showAboutWindow = false;

gui::debug::cpu::CPU cpuDebug;
gui::debug::timers::Timers timersDebug;

void aboutWindow() {
    ImGui::Begin("About", &showAboutWindow);
    ImGui::Text("Avocado %s", BUILD_STRING);
    ImGui::Text("Build date: %s", BUILD_DATE);
    ImGui::End();
}

extern void ImGui::ShowDemoWindow(bool* p_open);

int busToken;
int toastTimer = 0;
std::string toastMessage;

void initGui() {
    busToken = bus.listen<Event::Gui::Toast>([](auto e) {
        toastMessage = e.message;
        toastTimer = 2000;
    });
}

void deinitGui() { bus.unlistenAll(busToken); }

void renderImgui(System* sys) {
    if (showGui) {
        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("Open")) gui::file::openFile();
                if (ImGui::MenuItem("Exit", "Esc")) bus.notify(Event::File::Exit{});
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Emulation")) {
                if (ImGui::MenuItem("Pause/Resume", "Space")) {
                    if (sys->state == System::State::pause) {
                        sys->state = System::State::run;
                    } else if (sys->state == System::State::run) {
                        sys->state = System::State::pause;
                    }
                }
                if (ImGui::MenuItem("Soft reset", "F2")) bus.notify(Event::System::SoftReset{});
                if (ImGui::MenuItem("Hard reset", "Shift+F2")) bus.notify(Event::System::HardReset{});

                const char* shellStatus = sys->cdrom->getShell() ? "Close disk tray" : "Open disk tray";
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
                if (ImGui::MenuItem("System log", nullptr, &sys->debugOutput)) {
                    config["debug"]["log"]["system"] = (int)sys->debugOutput;
                }
                if (ImGui::MenuItem("BIOS calls log", nullptr, (bool*)&sys->biosLog)) {
                    config["debug"]["log"]["bios"] = sys->biosLog;
                }
#ifdef ENABLE_IO_LOG
                ImGui::MenuItem("IO log", nullptr, &ioLogEnabled);
#endif
                ImGui::MenuItem("GTE log", nullptr, &gteLogEnabled);
                ImGui::MenuItem("GPU log", nullptr, &gpuLogEnabled);

                ImGui::Separator();

                ImGui::MenuItem("GTE registers", nullptr, &gteRegistersEnabled);
                ImGui::MenuItem("Timers", nullptr, &timersDebug.timersWindowOpen);
                ;
                ImGui::MenuItem("Memory", nullptr, &showRamWindow);
                ImGui::MenuItem("VRAM", nullptr, &showVramWindow);
                ImGui::MenuItem("Debugger", nullptr, &cpuDebug.debuggerWindowOpen);
                ImGui::MenuItem("Breakpoints", nullptr, &cpuDebug.breakpointsWindowOpen);
                ImGui::MenuItem("Watch", nullptr, &showWatchWindow);
                ImGui::MenuItem("CDROM", nullptr, &showCdromWindow);
                ImGui::MenuItem("Kernel", nullptr, &showKernelWindow);
                ImGui::MenuItem("GPU", nullptr, &showGpuWindow);
                ImGui::MenuItem("SPU", nullptr, &showSpuWindow);
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Options")) {
                if (ImGui::MenuItem("Graphics", nullptr)) showGraphicsOptionsWindow = true;
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
        gui::file::displayWindows();

        // Debug
        if (gteRegistersEnabled) gteRegistersWindow(sys->cpu->gte);
        if (ioLogEnabled) ioLogWindow(sys);
        if (gteLogEnabled) gteLogWindow(sys);
        if (gpuLogEnabled) gpuLogWindow(sys);
        if (showRamWindow) ramWindow(sys);
        if (showVramWindow) vramWindow(sys->gpu.get());
        if (showWatchWindow) watchWindow(sys);
        if (showCdromWindow) cdromWindow(sys);
        if (showKernelWindow) kernelWindow(sys);
        if (showGpuWindow) gpuWindow(sys);
        if (showSpuWindow) spuWindow(sys->spu.get());

        timersDebug.displayWindows(sys);
        cpuDebug.displayWindows(sys);

        // Options
        if (showGraphicsOptionsWindow) graphicsOptionsWindow();
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

    if (toastTimer > 0) {
        // TODO: Do not use popup!
        //        ImGui::OpenPopup("##toast");
        //        if (ImGui::BeginPopup("##toast")) {
        //            ImGui::TextUnformatted(toastMessage.c_str());
        //            ImGui::EndPopup();
        //        }
        toastTimer--;
    }

    ImGui::Render();
    ImGui_ImplSdlGL3_RenderDrawData(ImGui::GetDrawData());
}
