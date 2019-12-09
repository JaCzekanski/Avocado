#include "gui.h"
#include <fmt/core.h>
#include <imgui.h>
#include "config.h"
#include "imgui/imgui_impl_opengl3.h"
#include "imgui/imgui_impl_sdl.h"
#include "platform/windows/input/key.h"
#include "system.h"
#include "utils/file.h"
#include "utils/string.h"

GUI::GUI(SDL_Window* window, void* glContext) : window(window) {
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    {
        ImGuiIO& io = ImGui::GetIO();
        ImGuiStyle& style = ImGui::GetStyle();

        ImFontConfig config;
        config.OversampleH = 4;
        config.OversampleV = 4;
        config.FontDataOwnedByAtlas = false;
        int fontSize = 16.f;

#ifdef ANDROID
        fontSize = 40.f;
        style.ScrollbarSize = 40.f;
        style.GrabMinSize = 20.f;
        style.TouchExtraPadding = ImVec2(10.f, 10.f);
#endif

        style.GrabRounding = 6.f;
        style.FrameRounding = 6.f;

        auto font = getFileContents("data/assets/roboto-mono.ttf");
        io.Fonts->AddFontFromMemoryTTF(font.data(), font.size(), fontSize, &config);
        io.Fonts->AddFontDefault();
    }

    ImGui_ImplSDL2_InitForOpenGL(window, glContext);
    ImGui_ImplOpenGL3_Init();
}

GUI::~GUI() { ImGui::DestroyContext(); }

void GUI::processEvent(SDL_Event* e) { ImGui_ImplSDL2_ProcessEvent(e); }

void GUI::mainMenu(System* sys) {
    if (!ImGui::BeginMainMenuBar()) {
        return;
    }
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
        ImGui::MenuItem("IO log", nullptr, &ioDebug.logWindowOpen);
#endif
        ImGui::MenuItem("GTE log", nullptr, &gteDebug.logWindowOpen);
        ImGui::MenuItem("GPU log", nullptr, &gpuDebug.logWindowOpen);

        ImGui::Separator();

        ImGui::MenuItem("Debugger", nullptr, &cpuDebug.debuggerWindowOpen);
        ImGui::MenuItem("Breakpoints", nullptr, &cpuDebug.breakpointsWindowOpen);
        ImGui::MenuItem("Watch", nullptr, &cpuDebug.watchWindowOpen);
        ImGui::MenuItem("RAM", nullptr, &cpuDebug.ramWindowOpen);

        ImGui::Separator();

        ImGui::MenuItem("CDROM", nullptr, &cdromDebug.cdromWindowOpen);
        ImGui::MenuItem("Timers", nullptr, &timersDebug.timersWindowOpen);
        ImGui::MenuItem("GPU", nullptr, &gpuDebug.registersWindowOpen);
        ImGui::MenuItem("GTE", nullptr, &gteDebug.registersWindowOpen);
        ImGui::MenuItem("SPU", nullptr, &spuDebug.spuWindowOpen);
        ImGui::MenuItem("VRAM", nullptr, &gpuDebug.vramWindowOpen);
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
        ImGui::MenuItem("About", nullptr, &aboutHelp.aboutWindowOpen);
        ImGui::MenuItem("System Information", nullptr, &sysinfoHelp.sysinfoWindowOpen);
        ImGui::EndMenu();
    }
    ImGui::EndMainMenuBar();
}

void GUI::render(System* sys) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame(window);
    ImGui::NewFrame();

    if (showGui) {
        mainMenu(sys);

        // File
        gui::file::displayWindows();

        // Debug
        if (showKernelWindow) kernelWindow(sys);

        cdromDebug.displayWindows(sys);
        cpuDebug.displayWindows(sys);
        gpuDebug.displayWindows(sys);
        timersDebug.displayWindows(sys);
        gteDebug.displayWindows(sys);
        spuDebug.displayWindows(sys);
        ioDebug.displayWindows(sys);

        // Options
        if (showGraphicsOptionsWindow) graphicsOptionsWindow();
        if (showBiosWindow) biosSelectionWindow();
        if (showControllerSetupWindow) controllerSetupWindow();

        memoryCardOptions.displayWindows(sys);

        // Help
        aboutHelp.displayWindows();
        sysinfoHelp.displayWindows();
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

    toasts.display();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}
