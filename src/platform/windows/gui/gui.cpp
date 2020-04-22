#include "gui.h"
#include <fmt/core.h>
#include <imgui.h>
#include "config.h"
#include "imgui/imgui_impl_opengl3.h"
#include "imgui/imgui_impl_sdl.h"
#include "platform/windows/input/key.h"
#include "platform/windows/gui/icons.h"
#include "system.h"
#include "utils/file.h"
#include "utils/string.h"

GUI::GUI(SDL_Window* window, void* glContext) : window(window) {
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
#ifdef ANDROID
    io.ConfigFlags |= ImGuiConfigFlags_IsTouchScreen;
#endif

    ImGuiStyle& style = ImGui::GetStyle();
#ifdef ANDROID
    style.ScrollbarSize = 40.f;
    style.GrabMinSize = 20.f;
    style.TouchExtraPadding = ImVec2(10.f, 10.f);
#endif
    style.GrabRounding = 6.f;
    style.FrameRounding = 6.f;

    ImFontConfig config;
    config.OversampleH = 2;
    config.OversampleV = 2;
    config.FontDataOwnedByAtlas = false;
    float fontSize = 16.f;
#ifdef ANDROID
    fontSize = 40.f;
#endif

    {
        auto font = getFileContents("data/assets/roboto-mono.ttf");
        io.Fonts->AddFontFromMemoryTTF(font.data(), font.size(), fontSize, &config);
    }

    io.Fonts->AddFontDefault();

    {
        auto font = getFileContents("data/assets/fa-solid-900.ttf");
        int fontSize = 16.f;
        ImFontConfig config;
        config.OversampleH = 2;
        config.OversampleV = 2;
        config.FontDataOwnedByAtlas = false;

        const char* glyphs[] = {
            ICON_FA_PLAY, ICON_FA_PAUSE, ICON_FA_SAVE, ICON_FA_COMPACT_DISC, ICON_FA_GAMEPAD, ICON_FA_COMPRESS, ICON_FA_COG,
        };

        ImVector<ImWchar> ranges;
        ImFontGlyphRangesBuilder builder;
        for (auto glyph : glyphs) {
            builder.AddText(glyph);
        }
        builder.BuildRanges(&ranges);
        io.Fonts->AddFontFromMemoryTTF(font.data(), font.size(), fontSize, &config, ranges.Data);
        io.Fonts->Build();
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
        ImGui::MenuItem("Open", nullptr, &openFile.openWindowOpen);
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
            config.debug.log.system = (int)sys->debugOutput;
        }
        if (ImGui::MenuItem("Syscall log", nullptr, (bool*)&sys->biosLog)) {
            config.debug.log.bios = sys->biosLog;
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
        ImGui::MenuItem("BIOS", nullptr, &biosOptions.biosWindowOpen);
        if (ImGui::MenuItem("Controller", nullptr)) showControllerSetupWindow = true;
        ImGui::MenuItem("Memory Card", nullptr, &memoryCardOptions.memoryCardWindowOpen);

        bool soundEnabled = config.options.sound.enabled;
        if (ImGui::MenuItem("Sound", nullptr, &soundEnabled)) {
            config.options.sound.enabled = soundEnabled;
            bus.notify(Event::Config::Spu{});
        }

        ImGui::Separator();

        bool preserveState = config.options.emulator.preserveState;
        if (ImGui::MenuItem("Save state on close", nullptr, &preserveState)) {
            config.options.emulator.preserveState = preserveState;
        }

        bool timeTravel = config.options.emulator.timeTravel;
        if (ImGui::MenuItem("Time travel", "backspace", &timeTravel)) {
            config.options.emulator.timeTravel = timeTravel;
        }

        ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Help")) {
        ImGui::MenuItem("About", nullptr, &aboutHelp.aboutWindowOpen);
        ImGui::EndMenu();
    }
    ImGui::EndMainMenuBar();
}

void GUI::render(System* sys) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame(window);
    ImGui::NewFrame();

    if (ImGui::BeginPopupModal("Avocado", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Avocado needs to be set up before running.");
        ImGui::Text("You need one of BIOS files placed in data/bios directory.");

        if (ImGui::Button("Select BIOS file")) {
            biosOptions.biosWindowOpen = true;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }

    if (showGui) {
        if (showMenu) mainMenu(sys);

        // File
        openFile.displayWindows();

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
        if (showControllerSetupWindow) controllerSetupWindow();

        biosOptions.displayWindows();
        memoryCardOptions.displayWindows(sys);

        // Help
        aboutHelp.displayWindows();
    }

    if (!config.isEmulatorConfigured() && !notInitializedWindowShown) {
        notInitializedWindowShown = true;
        ImGui::OpenPopup("Avocado");
    }

    toasts.display();

    drawControls(sys);

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void GUI::drawControls(System* sys) {
    auto symbolButton = [](const char* hint, const char* symbol, ImVec4 bg = ImVec4(1, 1, 1, 0.08f)) -> bool {
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10.f, 6.f));
        ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[2]);
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(bg.x, bg.y, bg.z, bg.w));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(bg.x, bg.y, bg.z, 0.5));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(bg.x, bg.y, bg.z, 0.75));

        bool clicked = ImGui::Button(symbol);

        ImGui::PopStyleColor(3);
        ImGui::PopFont();
        ImGui::PopStyleVar();

        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%s", hint);
        }

        return clicked;
    };
    auto io = ImGui::GetIO();

    static auto lastTime = std::chrono::steady_clock::now();
    // Reset timer if mouse was moved
    if (io.MouseDelta.x != 0 || io.MouseDelta.y != 0) {
        lastTime = std::chrono::steady_clock::now();
    }

    auto now = std::chrono::steady_clock::now();
    float timeDiff = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastTime).count() / 1000.f;

    const float visibleFor = 1.5f;       // Seconds
    const float fadeoutDuration = 0.5f;  // Seconds

    // Display for 1.5 seconds, then fade out 0.5 second, then hide
    float alpha;
    if (timeDiff < visibleFor) {
        alpha = 1.f;
    } else if (timeDiff < visibleFor + fadeoutDuration) {
        alpha = lerp(1.f, 0.f, (timeDiff - visibleFor) / fadeoutDuration);
    } else {
        return;
    }

    // TODO: Handle gamepad "home" button

    auto displaySize = io.DisplaySize;
    auto pos = ImVec2(displaySize.x / 2.f, displaySize.y * 0.9f);
    ImGui::SetNextWindowPos(pos, 0, ImVec2(.5f, .5f));

    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.f);
    ImGui::Begin("##controls", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize);

    ImVec4 pauseColor = ImVec4(1, 0, 0, 0.25f);
    if (sys->state == System::State::run) {
        if (symbolButton("Pause emulation", ICON_FA_PAUSE, pauseColor)) {
            sys->state = System::State::pause;
        }
    } else {
        if (symbolButton("Resume emulation", ICON_FA_PLAY, pauseColor)) {
            sys->state = System::State::run;
        }
    }

    ImGui::SameLine();

    symbolButton("Save/Load", ICON_FA_SAVE);
    if (ImGui::BeginPopupContextItem(nullptr, 0)) {
        if (ImGui::Selectable("Quick load")) bus.notify(Event::System::LoadState{});
        ImGui::Separator();
        if (ImGui::Selectable("Quick save")) bus.notify(Event::System::SaveState{});
        ImGui::EndPopup();
    }

    ImGui::SameLine();

    std::string game{};

    // Can lead to use-after-free when loading a new game and recrating the system
    if (sys->cdrom->disc) {
        auto cdPath = sys->cdrom->disc->getFile();
        if (!cdPath.empty()) {
            game = getFilename(cdPath);
        }
    }
    if (!game.empty()) {
        ImGui::TextUnformatted(game.c_str());
    } else {
        if (symbolButton("Open", ICON_FA_COMPACT_DISC, ImVec4(0.25f, 0.5f, 1, 0.4f))) {
            openFile.openWindowOpen = true;
        }
    }

    ImGui::SameLine();

    symbolButton("Settings", ICON_FA_COG);
    if (ImGui::BeginPopupContextItem(nullptr, 0)) {
        if (ImGui::Selectable("Controller")) showControllerSetupWindow = !showControllerSetupWindow;
        ImGui::Separator();
        ImGui::MenuItem("Show menu", "F1", &showMenu);
        ImGui::EndPopup();
    }

    ImGui::SameLine();

    if (symbolButton("Toggle fullscreen", ICON_FA_COMPRESS)) {
        bus.notify(Event::Gui::ToggleFullscreen{});
    }

    ImGui::End();
    ImGui::PopStyleVar();
    ImGui::PopStyleVar();
    ImGui::PopStyleVar();
}