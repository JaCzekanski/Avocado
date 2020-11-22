#include <SDL.h>
#include <fmt/core.h>
#include <imgui.h>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <string>
#include "config.h"
#include "config_parser.h"
#include "disc/load.h"
#include "gui/filesystem.h"
#include "gui/gui.h"
#include "input/sdl_input_manager.h"
#include "renderer/opengl/opengl.h"
#include "sound/sound.h"
#include "state/state.h"
#include "system.h"
#include "system_tools.h"
#include "utils/file.h"
#include "utils/string.h"
#include "utils/platform_tools.h"
#include "version.h"

#ifdef _WIN32
#include <direct.h>
#else
#include <unistd.h>
#endif

#undef main

// Warning: this method might have 1 or more milliseconds of inaccuracy.
double limitFramerate(bool framelimiter, bool ntsc) {
    static double timeToSkip = 0;
    static double counterFrequency = (double)SDL_GetPerformanceFrequency();
    static double startTime = SDL_GetPerformanceCounter() / counterFrequency;
    static double fpsTime = 0.0;
    static double fps = 0;
    static int deltaFrames = 0;

    double currentTime = SDL_GetPerformanceCounter() / counterFrequency;
    double deltaTime = currentTime - startTime;

    double frameTime = ntsc ? (1.0 / 60.0) : (1.0 / 50.0);

    if (framelimiter && deltaTime < frameTime) {
        // If deltaTime was shorter than frameTime - spin
        if (deltaTime < frameTime - timeToSkip) {
            while (deltaTime < frameTime - timeToSkip) {  // calculate real difference
                SDL_Delay(1);

                currentTime = SDL_GetPerformanceCounter() / counterFrequency;
                deltaTime = currentTime - startTime;
            }
            timeToSkip -= (frameTime - deltaTime);
            if (timeToSkip < 0.0) timeToSkip = 0.0;
        } else {  // Else - accumulate
            timeToSkip += deltaTime - frameTime;
        }
    }

    startTime = currentTime;
    fpsTime += deltaTime;
    deltaFrames++;

    if (fpsTime > 0.25f) {
        fps = (double)deltaFrames / fpsTime;
        deltaFrames = 0;
        fpsTime = 0.0;
    }

    return fps;
}

void fatalError(const std::string& error) {
    fmt::print(stderr, "[FATAL] {}", error);
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Avocado", error.c_str(), nullptr);
    SDL_Quit();
}

void changeWorkingDirectory() {
#if defined(ANDROID)
    avocado::PATH_DATA = "data";  // Search assets by default
#else
    fs::path workingDirectory = fs::current_path();

#if defined(BUILD_IS_RELEASE)
    char* basePath = SDL_GetBasePath();
    if (basePath != nullptr) {
        workingDirectory = basePath;
        SDL_free(basePath);
    }
#endif
    if (getenv("APPIMAGE") != nullptr) {
        workingDirectory = workingDirectory / ".." / "share" / "avocado";
    }

    workingDirectory /= "data";
    avocado::PATH_DATA = workingDirectory.string();
#endif

    auto ensureDirectoryExist = [](const fs::path& path) -> bool {
        if (fs::exists(path) && fs::is_directory(path)) {
            return true;
        }
        try {
            if (fs::create_directory(path)) {
                return true;
            }
        } catch (fs::filesystem_error& err) {
        }
        return false;
    };

#if defined(ANDROID)
    auto getAndroidExternalPath = [ensureDirectoryExist]() -> std::string {
        if (hasExternalStoragePermission() && (SDL_AndroidGetExternalStorageState() & SDL_ANDROID_EXTERNAL_STORAGE_WRITE) != 0) {
            // Try - /sdcard/avocado
            auto path = fs::path("/sdcard/avocado");
            if (ensureDirectoryExist(path)) {
                return path.string();
            }

            // Try external storage
            path = fs::path(SDL_AndroidGetExternalStoragePath());
            if (ensureDirectoryExist(path)) {
                return path.string();
            }
        }
        return SDL_AndroidGetInternalStoragePath();  // Almost the same as SDL_GetPrefPath
    };
    avocado::PATH_USER = getAndroidExternalPath();
#else
    char* prefPath = SDL_GetPrefPath(nullptr, "avocado");
    if (prefPath != nullptr) {
        avocado::PATH_USER = prefPath;
        SDL_free(prefPath);
    }
#endif
    avocado::PATH_DATA = replaceAll(avocado::PATH_DATA, "\\", "/");
    avocado::PATH_USER = replaceAll(avocado::PATH_USER, "\\", "/");

    if (!endsWith(avocado::PATH_DATA, "/")) {
        avocado::PATH_DATA += "/";
    }
    if (!endsWith(avocado::PATH_USER, "/")) {
        avocado::PATH_USER += "/";
    }

    std::string dirsToCreate[] = {
        avocado::biosPath(),
        avocado::statePath(),
        avocado::memoryPath(),
        avocado::isoPath(),
    };

    for (auto& d : dirsToCreate) {
        ensureDirectoryExist(fs::path(d));
    }

    chdir(avocado::PATH_USER.c_str());
}

int main(int argc, char** argv) {
    changeWorkingDirectory();

    loadConfigFile();

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER | SDL_INIT_JOYSTICK | SDL_INIT_AUDIO) != 0) {
        fatalError(fmt::format("Cannot init SDL ({})", SDL_GetError()));
        return 1;
    }

    SDL_GameControllerAddMappingsFromFile((avocado::assetsPath("gamecontrollerdb.txt")).c_str());

    auto opengl = std::make_unique<OpenGL>();

    SDL_Window* window = SDL_CreateWindow("Avocado", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, OpenGL::resWidth, OpenGL::resHeight,
                                          SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL | SDL_WINDOW_ALLOW_HIGHDPI);
    if (window == nullptr) {
        fatalError(fmt::format("Cannot create window ({})", SDL_GetError()));
        return 1;
    }

    SDL_GLContext glContext = SDL_GL_CreateContext(window);
    if (glContext == nullptr) {
        fatalError(fmt::format("Cannot create OpenGL context ({})", SDL_GetError()));
        return 1;
    }

    if (!opengl->setup()) {
        fatalError("Cannot setup graphics");
        return 1;
    }

    auto gui = std::make_unique<GUI>(window, glContext);
    Sound::init();

    std::unique_ptr<System> sys = system_tools::hardReset();

    int busToken = bus.listen<Event::File::Load>([&](auto e) {
        if (disc::isDiscImage(e.file)) {
            if (e.action == Event::File::Load::Action::ask) {
                // Show dialog and decide what to do
                gui->droppedItem = e.file;
                return;
            }

            std::unique_ptr<disc::Disc> disc = disc::load(e.file);
            if (!disc) {
                toast(fmt::format("Cannot load {}", getFilenameExt(e.file)));
                return;
            }

            if (e.action == Event::File::Load::Action::slowboot) {
                system_tools::bootstrap(sys);
                sys->cdrom->disc = std::move(disc);
                sys->cdrom->setShell(false);
                sys->state = System::State::run;

                toast("System restarted");
                return;
            } else if (e.action == Event::File::Load::Action::fastboot) {
                system_tools::bootstrap(sys);
                sys->cdrom->disc = std::move(disc);
                sys->cdrom->setShell(false);

                // BIOS is at 0x80030000 after bootstrap, forcing CPU to return
                // will skip the boot animation and go straight to the CD boot

                sys->cpu->setPC(sys->cpu->reg[31]);
                sys->state = System::State::run;

                toast("Fastboot");
                return;
            } else if (e.action == Event::File::Load::Action::swap) {
                sys->cdrom->disc = std::move(disc);
                sys->cdrom->setShell(false);

                toast("Disc swapped");
                return;
            }
        }

        bool isPaused = sys->state == System::State::pause;
        system_tools::loadFile(sys, e.file);
        if (isPaused && sys->state != System::State::pause) {
            sys->state = System::State::pause;
        }
    });

    bool exitProgram = false;
    bus.listen<Event::File::Exit>(busToken, [&](auto) { exitProgram = true; });
    bus.listen<Event::System::SoftReset>(busToken, [&](auto) { sys->softReset(); });
    bus.listen<Event::System::HardReset>(busToken, [&](auto) { sys = system_tools::hardReset(); });
    bus.listen<Event::System::SaveState>(busToken, [&](auto e) { state::quickSave(sys.get(), e.slot); });
    bus.listen<Event::System::LoadState>(busToken, [&](auto e) { state::quickLoad(sys.get(), e.slot); });

    bus.listen<Event::Gui::ToggleFullscreen>(busToken, [&](auto) {
        if (SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN_DESKTOP) {
            SDL_SetWindowFullscreen(window, 0);
        } else {
            SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
        }
    });

    bus.listen<Event::Config::Spu>(busToken, [&](auto) {
        bool soundEnabled = config.options.sound.enabled;
        if (soundEnabled) {
            Sound::play();
        } else {
            Sound::stop();
        }
    });

    if (config.options.sound.enabled) {
        Sound::play();
    }

    SDL_EventState(SDL_DROPFILE, SDL_ENABLE);
    SDL_GameControllerEventState(SDL_ENABLE);

    auto inputManager = std::make_unique<SdlInputManager>();
    InputManager::setInstance(inputManager.get());

    if (!sys->isSystemReady()) {
        sys->state = System::State::stop;
    } else {
        sys->state = System::State::run;

        // If argument given - open that file (same as drag and drop)
        if (argc > 1) {
            system_tools::loadFile(sys, argv[1]);
        } else {
            if (config.options.emulator.preserveState) {
                if (state::loadLastState(sys.get())) {
                    toast("Loaded last state");
                }
            }
        }
    }

    bool running = true;
    bool frameLimitEnabled = true;
    bool forceRedraw = false;

    SDL_Event event;
    while (running && !exitProgram) {
        bool newEvent = false;
        if (!forceRedraw && sys->state != System::State::run) {
            if (SDL_WaitEventTimeout(&event, 1000)) {
                newEvent = true;
            }
        }
        forceRedraw = false;

        auto lockMouse = sys->state == System::State::run && inputManager->mouseLocked;
        SDL_SetRelativeMouseMode((SDL_bool)lockMouse);

        inputManager->newFrame();
        while (newEvent || SDL_PollEvent(&event)) {
            gui->processEvent(&event);

            inputManager->keyboardCaptured = ImGui::GetIO().WantCaptureKeyboard;
            inputManager->mouseCaptured = ImGui::GetIO().WantCaptureMouse;

            newEvent = false;
            if (inputManager->handleEvent(event)) continue;
            if (event.type == SDL_QUIT || (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE)) running = false;

            Key button;
            bool controllerButtonDown = false;
            switch (event.type) {
                case SDL_KEYDOWN:
                    button = Key::keyboard(event.key.keysym.sym);
                    controllerButtonDown = true;
                    break;
                case SDL_CONTROLLERBUTTONDOWN:
                    button = Key::controllerButton(event.cbutton);
                    controllerButtonDown = true;
                    break;
                case SDL_CONTROLLERAXISMOTION:
                    button = Key::controllerMove(event.caxis);
                    if (button.controller.value) controllerButtonDown = true;
                    break;
            }

            const bool buttonPress = (event.type == SDL_KEYDOWN && !event.key.repeat) || controllerButtonDown;
            if (!inputManager->keyboardCaptured && buttonPress) {
                if (event.key.keysym.sym == SDLK_ESCAPE) running = false;
                if (event.key.keysym.sym == SDLK_AC_BACK) running = false;
                if (button == Key(config.hotkeys["toggle_menu"])) gui->showMenu = !gui->showMenu;
                if (button == Key(config.hotkeys["reset"])) {
                    if (event.key.keysym.mod & KMOD_SHIFT) {
                        sys = system_tools::hardReset();
                        toast("Hard reset");
                    } else {
                        sys->softReset();
                        toast("Soft reset");
                    }
                }
                if (button == Key(config.hotkeys["close_tray"])) {
                    sys->cdrom->toggleShell();
                    toast(fmt::format("Shell {}", sys->cdrom->getShell() ? "open" : "closed"));
                }
                if (button == Key(config.hotkeys["quick_save"])) {
                    bus.notify(Event::System::SaveState{});
                }
                if (button == Key(config.hotkeys["single_frame"])) {
                    gui->singleFrame = true;
                    sys->state = System::State::run;
                }
                if (button == Key(config.hotkeys["quick_load"])) {
                    bus.notify(Event::System::LoadState{});
                }
                if (button == Key(config.hotkeys["single_step"])) {
                    sys->singleStep();
                }
                if (button == Key(config.hotkeys["toggle_pause"])) {
                    if (sys->state == System::State::pause) {
                        sys->state = System::State::run;
                    } else if (sys->state == System::State::run) {
                        sys->state = System::State::pause;
                    }
                    toast(fmt::format("Emulation {}", sys->state == System::State::run ? "resumed" : "paused"));
                }
                if (button == Key(config.hotkeys["toggle_framelimit"])) {
                    frameLimitEnabled = !frameLimitEnabled;
                    toast(fmt::format("Frame limiter {}", frameLimitEnabled ? "enabled" : "disabled"));
                }
                if (button == Key(config.hotkeys["rewind_state"])) {
                    if (state::rewindState(sys.get())) {
                        toast("Going back 1 second");
                    }
                }
                if (button == Key(config.hotkeys["toggle_fullscreen"])) {
                    bus.notify(Event::Gui::ToggleFullscreen{});
                }
            }
            if (event.type == SDL_DROPFILE) {
                std::string path = event.drop.file;
                SDL_free(event.drop.file);

                bus.notify(Event::File::Load{path});
            }
            if (event.type == SDL_WINDOWEVENT
                && (event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED || event.window.event == SDL_WINDOWEVENT_RESIZED)) {
                opengl->width = event.window.data1;
                opengl->height = event.window.data2;
            }

            // Crude hack to force next frame after last event received (in paused mode)
            // Fixes ImGui window drawing
            forceRedraw = true;
        }

        if (sys->state == System::State::run) {
            sys->gpu->clear();
            sys->controller->update();

            sys->emulateFrame();
            if (gui->singleFrame) {
                gui->singleFrame = false;
                sys->state = System::State::pause;
            }

            state::manageTimeTravel(sys.get());
        }

        SDL_GL_GetDrawableSize(window, &opengl->width, &opengl->height);
        opengl->render(sys->gpu.get());

        gui->statusFramelimitter = frameLimitEnabled;
        gui->statusMouseLocked = inputManager->mouseLocked;
        gui->render(sys);

        SDL_GL_SwapWindow(window);

        gui->statusFps = limitFramerate(frameLimitEnabled, sys->gpu->isNtsc());
    }
    if (config.options.emulator.preserveState && sys->state != System::State::halted) {
        state::saveLastState(sys.get());
    }
    system_tools::saveMemoryCards(sys, true);
    saveConfigFile();

    bus.unlistenAll(busToken);
    Sound::close();
    InputManager::setInstance(nullptr);
    SDL_GL_DeleteContext(glContext);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
