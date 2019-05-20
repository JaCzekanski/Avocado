// Uncomment for leak detection
// #include <vld.h>

#include <SDL.h>
#include <algorithm>
#include <cstdio>
#include <string>
#include "bios/exe_bootstrap.h"
#include "config.h"
#include "disc/format/chd_format.h"
#include "disc/format/cue_parser.h"
#include "imgui/imgui_impl_sdl_gl3.h"
#include "platform/windows/gui/gui.h"
#include "platform/windows/input/sdl_input_manager.h"
#include "renderer/opengl/opengl.h"
#include "sound/sound.h"
#include "system.h"
#include "utils/file.h"
#include "utils/psf.h"
#include "utils/string.h"
#include "version.h"

#include <unistd.h>

#undef main

bool running = true;

void bootstrap(std::unique_ptr<System>& sys) {
    Sound::clearBuffer();
    sys = std::make_unique<System>();
    sys->loadBios(config["bios"]);
    sys->loadExpansion(exe_bootstrap);

    // Execute BIOS till breakpoint hit (shell is about to be executed)
    while (sys->state == System::State::run) sys->emulateFrame();
}

void loadFile(std::unique_ptr<System>& sys, std::string path) {
    std::string ext = getExtension(path);
    transform(ext.begin(), ext.end(), ext.begin(), tolower);

    std::string filenameExt = getFilenameExt(path);
    transform(filenameExt.begin(), filenameExt.end(), filenameExt.begin(), tolower);

    if (ext == "psf" || ext == "minipsf") {
        bootstrap(sys);
        loadPsf(sys.get(), path);
        sys->state = System::State::run;
        return;
    }

    if (ext == "exe" || ext == "psexe") {
        bootstrap(sys);
        // Replace shell with .exe contents
        sys->loadExeFile(getFileContents(path));

        // Resume execution
        sys->state = System::State::run;
        return;
    }

    if (ext == "json") {
        auto file = getFileContents(path);
        if (file.empty()) {
            return;
        }
        nlohmann::json j = json::parse(file.begin(), file.end());

        auto& gpuLog = sys->gpu->gpuLogList;
        gpuLog.clear();
        for (size_t i = 0; i < j.size(); i++) {
            gpu::LogEntry e;
            e.command = j[i]["command"];
            e.cmd = (gpu::Command)(int)j[i]["cmd"];

            for (uint32_t a : j[i]["args"]) {
                e.args.push_back(a);
            }

            gpuLog.push_back(e);
        }
        return;
    }

    std::unique_ptr<disc::Disc> disc;
    if (ext == "chd") {
        disc = disc::format::Chd::open(path);
    } else if (ext == "cue") {
        disc::format::CueParser parser;
        disc = parser.parse(path.c_str());
    } else if (ext == "iso" || ext == "bin" || ext == "img") {
        disc = disc::format::Cue::fromBin(path.c_str());
    }

    if (disc) {
        sys->cdrom->disc = std::move(disc);
        sys->cdrom->setShell(false);
        printf("File %s loaded\n", filenameExt.c_str());
    }
}

void saveMemoryCards(std::unique_ptr<System>& sys, bool force = false) {
    if (!force && !sys->controller->card[0]->dirty) return;

    std::string pathCard1 = config["memoryCard"]["1"];
    if (pathCard1.empty()) {
        printf("[INFO] No memory card 1 path in config, skipping save\n");
        return;
    }

    auto& data = sys->controller->card[0]->data;
    auto output = std::vector<uint8_t>(data.begin(), data.end());

    if (!putFileContents(pathCard1, output)) {
        printf("[INFO] Unable to save memory card 1 to %s\n", getFilenameExt(pathCard1).c_str());
        return;
    }

    printf("[INFO] Saved memory card 1 to %s\n", getFilenameExt(pathCard1).c_str());
}

std::unique_ptr<System> hardReset() {
    auto sys = std::make_unique<System>();

    std::string bios = config["bios"];
    if (!bios.empty() && sys->loadBios(bios)) {
        printf("[INFO] Using bios %s\n", getFilenameExt(bios).c_str());
    }

    std::string extension = config["extension"];
    if (!extension.empty() && sys->loadExpansion(getFileContents(extension))) {
        printf("[INFO] Using extension %s\n", getFilenameExt(extension).c_str());
    }

    std::string iso = config["iso"];
    if (!iso.empty()) {
        loadFile(sys, iso);
        printf("Using iso %s\n", iso.c_str());
    }

    std::string pathCard1 = config["memoryCard"]["1"];
    if (!pathCard1.empty()) {
        auto card1 = getFileContents(pathCard1);
        if (!card1.empty()) {
            std::copy_n(std::make_move_iterator(card1.begin()), card1.size(), sys->controller->card[0]->data.begin());
            printf("[INFO] Loaded memory card 1 from %s\n", getFilenameExt(pathCard1).c_str());
        }
    }
    return sys;
}

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

    double frameTime = ntsc ? (1.0 / 60.0) : (1.0 / 50.0);

    // Hack: when emulation is paused and app is inactive
    // detlaTime accumulates and forces app to run without framelimiting
    // until this time passes.
    // This limits that behavior
    if (deltaTime > 1.0) {
        deltaTime = 1.0;
    }

    if (framelimiter) {
        // If frame was shorter than frameTime - spin
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

        // TODO: Move this part outside method
        std::string gameName = "No CD";
        if (sys->cdrom->disc) {
            gameName = getFilename(sys->cdrom->disc->getFile());
        }

        std::string title = string_format("Avocado %s | %s | FPS: %.0f (%0.2f ms) %s", BUILD_STRING, gameName.c_str(), fps,
                                          (1.0 / fps) * 1000.0, !framelimiter ? "unlimited" : "");
        if (mouseLocked) {
            title = "Press Alt to unlock mouse | " + title;
        }
        SDL_SetWindowTitle(window, title.c_str());
    }
}

int main(int argc, char** argv) {
#ifdef ANDROID
    chdir("/sdcard/avocado");
#endif
    loadConfigFile(CONFIG_NAME);

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER | SDL_INIT_JOYSTICK | SDL_INIT_AUDIO) != 0) {
        printf("Cannot init SDL (%s)\n", SDL_GetError());
        return 1;
    }

    SDL_GameControllerAddMappingsFromFile("data/assets/gamecontrollerdb.txt");

    OpenGL opengl;
    if (!opengl.init()) {
        printf("Cannot initialize OpenGL\n");
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow("Avocado", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, OpenGL::resWidth, OpenGL::resHeight,
                                          SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL | SDL_WINDOW_ALLOW_HIGHDPI);
    if (window == nullptr) {
        printf("Cannot create window (%s)\n", SDL_GetError());
        return 1;
    }

    SDL_GLContext glContext = SDL_GL_CreateContext(window);
    if (glContext == nullptr) {
        printf("Cannot create OpenGL context (%s)\n", SDL_GetError());
        return 1;
    }

    if (!opengl.setup()) {
        printf("Cannot setup graphics\n");
        return 1;
    }

    Sound::init();

    std::unique_ptr<System> sys = hardReset();

    // If argument given - open that file (same as drag and drop)
    if (argc > 1) {
        loadFile(sys, argv[1]);
    }

    SDL_EventState(SDL_DROPFILE, SDL_ENABLE);
    SDL_GameControllerEventState(SDL_ENABLE);

    auto inputManager = std::make_unique<SdlInputManager>();
    InputManager::setInstance(inputManager.get());

    ImGui::CreateContext();
    ImGui_ImplSdlGL3_Init(window);
    ImGui::StyleColorsDark();
    {
        ImGuiIO& io = ImGui::GetIO();
        // io.Fonts->AddFontDefault();
        ImFontConfig config;
        config.OversampleH = 4;
        config.OversampleV = 4;
        io.Fonts->AddFontFromFileTTF("data/assets/roboto-mono.ttf", 16.0f, &config);
        io.Fonts->AddFontDefault();

#ifdef ANDROID
        io.FontGlobalScale = 3.f;
#endif
    }

    ImGuiStyle& style = ImGui::GetStyle();
    style.GrabRounding = style.FrameRounding = 6.f;

    Sound::play();

    if (!isEmulatorConfigured())
        sys->state = System::State::stop;
    else
        sys->state = System::State::run;

    bool frameLimitEnabled = true;
    bool windowFocused = true;

    SDL_Event event;
    while (running && !exitProgram) {
        bool newEvent = false;
        if (sys->state != System::State::run && !windowFocused) {
            SDL_WaitEvent(&event);
            newEvent = true;
        }

        auto lockMouse = sys->state == System::State::run && inputManager->mouseLocked;
        SDL_SetRelativeMouseMode((SDL_bool)lockMouse);

        inputManager->newFrame();
        while (newEvent || SDL_PollEvent(&event)) {
            ImGui_ImplSdlGL3_ProcessEvent(&event);

            newEvent = false;
            if (inputManager->handleEvent(event)) continue;
            if (event.type == SDL_QUIT || (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE)) running = false;
            if (event.type == SDL_WINDOWEVENT) {
                if (event.window.event == SDL_WINDOWEVENT_FOCUS_LOST) windowFocused = false;
                if (event.window.event == SDL_WINDOWEVENT_FOCUS_GAINED) windowFocused = true;
            }
            if (event.type == SDL_KEYDOWN && event.key.repeat == 0) {
                if (event.key.keysym.sym == SDLK_ESCAPE) {
                    running = false;
                }
                if (event.key.keysym.sym == SDLK_r) {
                    sys->dumpRam();
                    sys->spu->dumpRam();
                }
                if (event.key.keysym.sym == SDLK_F1) showGui = !showGui;
                if (event.key.keysym.sym == SDLK_F2) {
                    if (event.key.keysym.mod & KMOD_SHIFT) {
                        sys = hardReset();
                    } else
                        sys->softReset();
                }
                if (event.key.keysym.sym == SDLK_F3) {
                    printf("Shell toggle\n");
                    sys->cdrom->toggleShell();
                }
                if (event.key.keysym.sym == SDLK_F7) {
                    singleFrame = true;
                    sys->state = System::State::run;
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
                }
                if (event.key.keysym.sym == SDLK_TAB) frameLimitEnabled = !frameLimitEnabled;
            }
            if (event.type == SDL_DROPFILE) {
                std::string path = event.drop.file;
                SDL_free(event.drop.file);
                loadFile(sys, path);
            }
            if (event.type == SDL_WINDOWEVENT
                && (event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED || event.window.event == SDL_WINDOWEVENT_RESIZED)) {
                opengl.width = event.window.data1;
                opengl.height = event.window.data2;
            }
        }

        if (doHardReset) {
            doHardReset = false;
            sys = hardReset();
        }

        if (sys->state == System::State::run) {
            sys->gpu->clear();
            sys->controller->update();

            sys->emulateFrame();
            if (singleFrame) {
                singleFrame = false;
                sys->state = System::State::pause;
            }
        }
        ImGui_ImplSdlGL3_NewFrame(window);

        SDL_GL_GetDrawableSize(window, &opengl.width, &opengl.height);
        opengl.render(sys->gpu.get());
        renderImgui(sys.get());

        SDL_GL_SwapWindow(window);

        limitFramerate(sys, window, frameLimitEnabled, sys->gpu->isNtsc(), inputManager->mouseLocked);
    }
    saveMemoryCards(sys, true);
    saveConfigFile(CONFIG_NAME);

    Sound::close();
    ImGui_ImplSdlGL3_Shutdown();
    ImGui::DestroyContext();
    InputManager::setInstance(nullptr);
    opengl.~OpenGL();
    SDL_GL_DeleteContext(glContext);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
