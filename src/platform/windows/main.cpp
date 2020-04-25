#include <SDL.h>
#include <fmt/core.h>
#include <imgui.h>
#include <algorithm>
#include <cstdio>
#include <string>
#include "config.h"
#include "config_parser.h"
#include "gui/gui.h"
#include "input/sdl_input_manager.h"
#include "renderer/opengl/opengl.h"
#include "sound/sound.h"
#include "state/state.h"
#include "system.h"
#include "system_tools.h"
#include "utils/file.h"

#ifdef _WIN32
#include <direct.h>
#else
#include <unistd.h>
#endif

#undef main

bool running = true;

// Warning: this method might have 1 or more miliseconds of inaccuracy.
void limitFramerate(std::unique_ptr<System>& sys, SDL_Window* window, bool framelimiter, bool ntsc, bool mouseLocked) {
    static double timeToSkip = 0;
    static double counterFrequency = (double)SDL_GetPerformanceFrequency();
    static double startTime = SDL_GetPerformanceCounter() / counterFrequency;
    static double fps;
    static double fpsTime = 0.0;
    static int deltaFrames = 0;

    double currentTime = SDL_GetPerformanceCounter() / counterFrequency;
    double deltaTime = currentTime - startTime;

#ifdef USE_EXACT_FPS
    double frameTime = ntsc ? (1.0 / 59.29) : (1.0 / 49.76);
#else
    double frameTime = ntsc ? (1.0 / 60.0) : (1.0 / 50.0);
#endif

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

        std::string title = "";
        if (mouseLocked) {
            title += "Press Alt to unlock mouse | ";
        }

        title += "Avocado";

        if (sys->cdrom->disc) {
            auto cdPath = sys->cdrom->disc->getFile();
            if (!cdPath.empty()) {
                title += " | " + getFilename(cdPath);
            }
        }

        title += fmt::format(" | FPS: {:.0f} ({:.2f} ms) {}", fps, (1.0 / fps) * 1000.0, !framelimiter ? "unlimited" : "");

        if (sys->state == System::State::pause) {
            title += " | paused";
        }
        SDL_SetWindowTitle(window, title.c_str());
    }
}

void fatalError(const std::string& error) {
    fmt::print(stderr, "[FATAL] %s", error);
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Avocado", error.c_str(), nullptr);
    SDL_Quit();
}

void changeWorkingDirectory() {
    std::string workingDirectory = ".";
#if defined(__linux__)
    return;  // AppImage, no change
#elif defined(ANDROID)
    workingDirectory = "/sdcard/avocado";
#else
    char* basePath = SDL_GetBasePath();
    workingDirectory = basePath;
    SDL_free(basePath);
#endif
    chdir(workingDirectory.c_str());
}

int main(int argc, char** argv) {
#ifdef BUILD_IS_RELEASE
    changeWorkingDirectory();
#endif

    loadConfigFile(CONFIG_NAME);

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER | SDL_INIT_JOYSTICK | SDL_INIT_AUDIO) != 0) {
        fatalError(fmt::format("Cannot init SDL ({})", SDL_GetError()));
        return 1;
    }

    SDL_GameControllerAddMappingsFromFile("data/assets/gamecontrollerdb.txt");

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

    Sound::init();

    std::unique_ptr<System> sys = system_tools::hardReset();

    int busToken = bus.listen<Event::File::Load>([&](auto e) {
        bool isPaused = sys->state == System::State::pause;
        if (e.reset) {
            sys = system_tools::hardReset();
        }
        system_tools::loadFile(sys, e.file);
        sys->state = isPaused ? System::State::pause : System::State::run;
    });

    bool exitProgram = false;
    bool doHardReset = false;
    bus.listen<Event::File::Exit>(busToken, [&](auto) { exitProgram = true; });
    bus.listen<Event::System::SoftReset>(busToken, [&](auto) { sys->softReset(); });
    bus.listen<Event::System::HardReset>(busToken, [&](auto) { doHardReset = true; });
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

    auto gui = std::make_unique<GUI>(window, glContext);

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

    bool frameLimitEnabled = true;
    bool windowFocused = true;

    bool forceRedraw = false;

    SDL_Event event;
    while (running && !exitProgram) {
        bool newEvent = false;
        if (!forceRedraw && sys->state != System::State::run) {
            SDL_WaitEvent(&event);
            newEvent = true;
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
            if (event.type == SDL_WINDOWEVENT) {
                if (event.window.event == SDL_WINDOWEVENT_FOCUS_LOST) windowFocused = false;
                if (event.window.event == SDL_WINDOWEVENT_FOCUS_GAINED) windowFocused = true;
            }
            if (event.type == SDL_AUDIODEVICEADDED || event.type == SDL_AUDIODEVICEREMOVED) {
                Sound::close();
                Sound::init();
                if (config.options.sound.enabled) {
                    Sound::play();
                }
            }
            if (!inputManager->keyboardCaptured && event.type == SDL_KEYDOWN && event.key.repeat == 0) {
                if (event.key.keysym.sym == SDLK_ESCAPE) running = false;
                if (event.key.keysym.sym == SDLK_F1) gui->showGui = !gui->showGui;
                if (event.key.keysym.sym == SDLK_F2) {
                    if (event.key.keysym.mod & KMOD_SHIFT) {
                        sys = system_tools::hardReset();
                        toast("Hard reset");
                    } else {
                        sys->softReset();
                        toast("Soft reset");
                    }
                }
                if (event.key.keysym.sym == SDLK_F3) {
                    sys->cdrom->toggleShell();
                    toast(fmt::format("Shell {}", sys->cdrom->getShell() ? "open" : "closed"));
                }
                if (event.key.keysym.sym == SDLK_F5) {
                    bus.notify(Event::System::SaveState{});
                }
                if (event.key.keysym.sym == SDLK_F6) {
                    gui->singleFrame = true;
                    sys->state = System::State::run;
                }
                if (event.key.keysym.sym == SDLK_F7) {
                    bus.notify(Event::System::LoadState{});
                }
                if (event.key.keysym.sym == SDLK_F8) {
                    sys->singleStep();
                }
                if (event.key.keysym.sym == SDLK_SPACE) {
                    if (sys->state == System::State::pause) {
                        sys->state = System::State::run;
                    } else if (sys->state == System::State::run) {
                        sys->state = System::State::pause;
                    }
                    toast(fmt::format("Emulation {}", sys->state == System::State::run ? "resumed" : "paused"));
                }
                if (event.key.keysym.sym == SDLK_TAB) {
                    frameLimitEnabled = !frameLimitEnabled;
                    toast(fmt::format("Frame limiter {}", frameLimitEnabled ? "enabled" : "disabled"));
                }
                if (event.key.keysym.sym == SDLK_BACKSPACE) {
                    if (state::rewindState(sys.get())) {
                        toast("Going back 1 second");
                    }
                }
            }
            if (event.type == SDL_DROPFILE) {
                std::string path = event.drop.file;
                SDL_free(event.drop.file);
                system_tools::loadFile(sys, path);
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

        if (doHardReset) {
            doHardReset = false;
            sys = system_tools::hardReset();
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

        gui->render(sys.get());

        SDL_GL_SwapWindow(window);

        limitFramerate(sys, window, frameLimitEnabled, sys->gpu->isNtsc(), inputManager->mouseLocked);
    }
    if (config.options.emulator.preserveState) {
        state::saveLastState(sys.get());
    }
    system_tools::saveMemoryCards(sys, true);
    saveConfigFile(CONFIG_NAME);

    bus.unlistenAll(busToken);
    Sound::close();
    InputManager::setInstance(nullptr);
    SDL_GL_DeleteContext(glContext);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
