#include "gui.h"
#include <fmt/core.h>
#include <imgui.h>
#include <chrono>
#include "config.h"
#include "debug/cdrom.h"
#include "debug/cpu.h"
#include "debug/gpu.h"
#include "debug/gte.h"
#include "debug/kernel.h"
#include "debug/spu.h"
#include "debug/timers.h"
#include "options.h"
#include "options/memory_card/memory_card.h"
#include "platform/windows/input/key.h"
#include "utils/string.h"
#include "version.h"

void openFileWindow();

void ioLogWindow(System* sys);
void gpuLogWindow(System* sys);
void vramWindow(gpu::GPU* gpu);
void watchWindow(System* sys);
void ramWindow(System* sys);

bool showGui = true;

bool ioLogEnabled = false;
bool gpuLogEnabled = false;
bool singleFrame = false;

bool notInitializedWindowShown = false;
bool showVramWindow = false;
bool showWatchWindow = false;
bool showRamWindow = false;
bool showSpuWindow = false;

bool showAboutWindow = false;

gui::debug::Cdrom cdromDebug;
gui::debug::CPU cpuDebug;
gui::debug::Timers timersDebug;
gui::debug::GTE gteDebug;
gui::debug::SPU spuDebug;

gui::options::memory_card::MemoryCard memoryCardOptions;

void aboutWindow() {
    ImGui::Begin("About", &showAboutWindow);
    ImGui::Text("Avocado %s", BUILD_STRING);
    ImGui::Text("Build date: %s", BUILD_DATE);
    ImGui::End();
}

extern void ImGui::ShowDemoWindow(bool* p_open);

int busToken;
using namespace std::chrono_literals;
using chrono = std::chrono::steady_clock;
struct Toast {
    std::string msg;
    chrono::time_point expire;
};
std::vector<Toast> toasts;

std::string getToasts() {
    std::string msg = "";
    if (!toasts.empty()) {
        auto now = chrono::now();

        for (auto it = toasts.begin(); it != toasts.end();) {
            auto toast = *it;

            if (now > toast.expire) {
                it = toasts.erase(it);
            } else {
                msg += toast.msg + "\n";
                ++it;
            }
        }
    }
    return trim(msg);
}

void initGui() {
    busToken = bus.listen<Event::Gui::Toast>([](auto e) {
        fmt::print("{}\n", e.message);
        toasts.push_back({e.message, chrono::now() + 2s});
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

                if (ImGui::MenuItem("Single frame", "F6")) {
                    singleFrame = true;
                    sys->state = System::State::run;
                }

                ImGui::Separator();

                if (ImGui::MenuItem("Quick save", "F5")) bus.notify(Event::System::SaveState{});
                if (ImGui::MenuItem("Quick load", "F7")) bus.notify(Event::System::LoadState{});

                if (ImGui::BeginMenu("Save")) {
                    for (int i = 1; i <= 5; i++) {
                        if (ImGui::MenuItem(fmt::format("Slot {}##save", i).c_str())) bus.notify(Event::System::SaveState{i});
                    }
                    ImGui::EndMenu();
                }

                if (ImGui::BeginMenu("Load")) {
                    for (int i = 1; i <= 5; i++) {
                        if (ImGui::MenuItem(fmt::format("Slot {}##load", i).c_str())) bus.notify(Event::System::LoadState{i});
                    }
                    ImGui::EndMenu();
                }

                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Debug")) {
                if (ImGui::MenuItem("System log", nullptr, &sys->debugOutput)) {
                    config["debug"]["log"]["system"] = (int)sys->debugOutput;
                }
                if (ImGui::MenuItem("Syscall log", nullptr, (bool*)&sys->biosLog)) {
                    config["debug"]["log"]["bios"] = sys->biosLog;
                }
#ifdef ENABLE_IO_LOG
                ImGui::MenuItem("IO log", nullptr, &ioLogEnabled);
#endif
                ImGui::MenuItem("GTE log", nullptr, &gteDebug.logWindowOpen);
                ImGui::MenuItem("GPU log", nullptr, &gpuLogEnabled);

                ImGui::Separator();

                ImGui::MenuItem("Debugger", nullptr, &cpuDebug.debuggerWindowOpen);
                ImGui::MenuItem("Breakpoints", nullptr, &cpuDebug.breakpointsWindowOpen);
                ImGui::MenuItem("Watch", nullptr, &showWatchWindow);
                ImGui::MenuItem("Memory", nullptr, &showRamWindow);

                ImGui::Separator();
                ImGui::MenuItem("CDROM", nullptr, &cdromDebug.cdromWindowOpen);
                ImGui::MenuItem("Timers", nullptr, &timersDebug.timersWindowOpen);
                ImGui::MenuItem("GPU", nullptr, &showGpuWindow);
                ImGui::MenuItem("GTE", nullptr, &gteDebug.registersWindowOpen);
                ImGui::MenuItem("SPU", nullptr, &spuDebug.spuWindowOpen);
                ImGui::MenuItem("VRAM", nullptr, &showVramWindow);
                ImGui::MenuItem("Kernel", nullptr, &showKernelWindow);

                ImGui::Separator();
                if (ImGui::MenuItem("Dump state")) {
                    sys->dumpRam();
                    sys->spu->dumpRam();
                    sys->gpu->dumpVram();
                    toast("State dumped");
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Options")) {
                if (ImGui::MenuItem("Graphics", nullptr)) showGraphicsOptionsWindow = true;
                if (ImGui::MenuItem("BIOS", nullptr)) showBiosWindow = true;
                if (ImGui::MenuItem("Controller", nullptr)) showControllerSetupWindow = true;
                ImGui::MenuItem("Memory Card", nullptr, &memoryCardOptions.memoryCardWindowOpen);

                bool soundEnabled = config["options"]["sound"]["enabled"];
                if (ImGui::MenuItem("Sound", nullptr, &soundEnabled)) {
                    config["options"]["sound"]["enabled"] = soundEnabled;
                    bus.notify(Event::Config::Spu{});
                }

                ImGui::Separator();

                bool preserveState = config["options"]["emulator"]["preserveState"];
                if (ImGui::MenuItem("Save state on close", nullptr, &preserveState)) {
                    config["options"]["emulator"]["preserveState"] = preserveState;
                }

                bool timeTravel = config["options"]["emulator"]["timeTravel"];
                if (ImGui::MenuItem("Time travel", "backspace", &timeTravel)) {
                    config["options"]["emulator"]["timeTravel"] = timeTravel;
                }

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
        if (ioLogEnabled) ioLogWindow(sys);
        if (gpuLogEnabled) gpuLogWindow(sys);
        if (showRamWindow) ramWindow(sys);
        if (showVramWindow) vramWindow(sys->gpu.get());
        if (showWatchWindow) watchWindow(sys);
        if (showKernelWindow) kernelWindow(sys);
        if (showGpuWindow) gpuWindow(sys);

        cdromDebug.displayWindows(sys);
        cpuDebug.displayWindows(sys);
        timersDebug.displayWindows(sys);
        gteDebug.displayWindows(sys);
        spuDebug.displayWindows(sys);

        // Options
        if (showGraphicsOptionsWindow) graphicsOptionsWindow();
        if (showBiosWindow) biosSelectionWindow();
        if (showControllerSetupWindow) controllerSetupWindow();

        memoryCardOptions.displayWindows(sys);

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

    if (auto toasts = getToasts(); !toasts.empty()) {
        const float margin = 16;
        auto displaySize = ImGui::GetIO().DisplaySize;

        auto pos = ImVec2(displaySize.x - margin, displaySize.y - margin);
        ImGui::SetNextWindowPos(pos, 0, ImVec2(1.f, 1.f));

        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
        ImGui::Begin("##toast", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::TextUnformatted(toasts.c_str());
        ImGui::End();
        ImGui::PopStyleVar();
    }

    ImGui::Render();
}
