// Uncomment for leak detection
// #include <vld.h>

#include <SDL.h>
#include <algorithm>
#include <cstdio>
#include <nlohmann/json.hpp>
#include <string>
#include "imgui/imgui_impl_sdl_gl3.h"
#include "platform/windows/config.h"
#include "platform/windows/gui/gui.h"
#include "renderer/opengl/opengl.h"
#include "sound/audio_cd.h"
#include "system.h"
#include "utils/cue/cueParser.h"
#include "utils/file.h"
#include "utils/string.h"
#include "version.h"

#undef main

const int CPU_CLOCK = 33868500;
const int GPU_CLOCK_NTSC = 53690000;

device::controller::DigitalController& getButtonState(SDL_Event& event) {
    static SDL_GameController* controller = nullptr;
    static device::controller::DigitalController buttons;
    if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) {
        for (auto it = config["controller"].begin(); it != config["controller"].end(); ++it) {
            auto button = it.key();
            auto keyName = it.value().get<std::string>();
            auto keyCode = SDL_GetKeyFromName(keyName.c_str());

            if (event.key.keysym.sym == keyCode) buttons.setByName(button, event.type == SDL_KEYDOWN);
        }
    }
    if (event.type == SDL_CONTROLLERDEVICEADDED) {
        controller = SDL_GameControllerOpen(event.cdevice.which);
        printf("Controller %s connected\n", SDL_GameControllerName(controller));
    }
    if (event.type == SDL_CONTROLLERDEVICEREMOVED) {
        printf("Controller %s disconnected\n", SDL_GameControllerName(controller));
        SDL_GameControllerClose(controller);
        controller = nullptr;
    }

    if (event.type == SDL_CONTROLLERAXISMOTION) {
        const int deadzone = 8 * 1024;
        switch (event.caxis.axis) {
            case SDL_CONTROLLER_AXIS_LEFTY:
                buttons.up = event.caxis.value < -deadzone;
                buttons.down = event.caxis.value > deadzone;
                break;

            case SDL_CONTROLLER_AXIS_LEFTX:
                buttons.left = event.caxis.value < -deadzone;
                buttons.right = event.caxis.value > deadzone;
                break;

            case SDL_CONTROLLER_AXIS_TRIGGERLEFT:
                buttons.l2 = event.caxis.value > 2048;
                break;

            case SDL_CONTROLLER_AXIS_TRIGGERRIGHT:
                buttons.r2 = event.caxis.value > 2048;
                break;

            default:
                break;
        }
    }

    if (event.type == SDL_CONTROLLERBUTTONDOWN || event.type == SDL_CONTROLLERBUTTONUP) {
#define B(a, b)                                   \
    case a:                                       \
        b = (event.cbutton.state == SDL_PRESSED); \
        break;
        switch (event.cbutton.button) {
            B(SDL_CONTROLLER_BUTTON_DPAD_UP, buttons.up);
            B(SDL_CONTROLLER_BUTTON_DPAD_RIGHT, buttons.right);
            B(SDL_CONTROLLER_BUTTON_DPAD_DOWN, buttons.down);
            B(SDL_CONTROLLER_BUTTON_DPAD_LEFT, buttons.left);
            B(SDL_CONTROLLER_BUTTON_A, buttons.cross);
            B(SDL_CONTROLLER_BUTTON_B, buttons.circle);
            B(SDL_CONTROLLER_BUTTON_X, buttons.square);
            B(SDL_CONTROLLER_BUTTON_Y, buttons.triangle);
            B(SDL_CONTROLLER_BUTTON_BACK, buttons.select);
            B(SDL_CONTROLLER_BUTTON_START, buttons.start);
            B(SDL_CONTROLLER_BUTTON_LEFTSHOULDER, buttons.l1);
            B(SDL_CONTROLLER_BUTTON_RIGHTSHOULDER, buttons.r1);
#undef B
            default:
                break;
        }
    }
    return buttons;
}

bool running = true;

void loadAssetsFromMakefile(std::unique_ptr<System>& sys, const std::string& basePath, const std::string& content) {
    auto datDirRegex = std::regex("DATDIR=(.*)");
    auto regex = std::regex("pqbload (.*) (.*)");

    if (content.empty()) return;

    std::string datDir;

    std::stringstream stream;
    stream.str(content);

    std::string line;
    while (std::getline(stream, line)) {
        std::smatch matches;

        std::regex_search(line, matches, datDirRegex);
        if (matches.size() == 2) {
            datDir = matches[1].str();
        }

        std::regex_search(line, matches, regex);
        if (matches.size() == 3) {
            std::string file = std::regex_replace(matches[1].str(), std::regex("\\$\\(DATDIR\\)"), datDir);
            uint32_t addr = std::stoul(matches[2].str(), 0, 16);

            printf("[INFO] Loading %s to 0x%x ", file.c_str(), addr);

            // Load data file
            auto data = getFileContents(basePath + file);
            if (data.empty()) {
                printf("failed\n");
                continue;
            }

            for (int i = 0; i < data.size(); i++) sys->writeMemory8(addr + i, data[i]);
            printf("ok\n");
        }
    }
}

void loadFile(std::unique_ptr<System>& sys, std::string path) {
    std::string ext = getExtension(path);
    transform(ext.begin(), ext.end(), ext.begin(), tolower);

    std::string filenameExt = getFilenameExt(path);
    transform(filenameExt.begin(), filenameExt.end(), filenameExt.begin(), tolower);

    if (filenameExt == "makefile.mak") {
        loadAssetsFromMakefile(sys, getPath(path), getFileContentsAsString(path));
        return;
    }

    if (ext == "exe" || ext == "psexe") {
        sys->loadExeFile(path);
        printf("Loading .exe is currently not supported.\n");
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
            GPU::GPU_LOG_ENTRY e;
            e.command = j[i]["command"];
            e.cmd = (Command)(int)j[i]["cmd"];

            for (uint32_t a : j[i]["args"]) {
                e.args.push_back(a);
            }

            gpuLog.push_back(e);
        }
        return;
    }

    std::unique_ptr<utils::Cue> cue = nullptr;

    if (ext == "cue") {
        try {
            utils::CueParser parser;
            cue = parser.parse(path.c_str());
        } catch (std::exception& e) {
            printf("Error parsing cue: %s\n", e.what());
        }
    } else if (ext == "iso" || ext == "bin" || ext == "img") {
        cue = utils::Cue::fromBin(path.c_str());
    }

    if (cue != nullptr) {
        sys->cdrom->cue = *cue;
        bool success = dynamic_cast<device::dma::dmaChannel::DMA3Channel*>(sys->dma->dma[3].get())->load(cue->tracks[0].filename);
        sys->cdrom->setShell(!success);
        printf("File %s loaded\n", filenameExt.c_str());
    }
}

std::unique_ptr<System> sys;

void hardReset() {
    sys = std::make_unique<System>();

    std::string bios = config["bios"];
    if (!bios.empty() && sys->loadBios(bios)) {
        printf("[INFO] Using bios %s\n", getFilenameExt(bios).c_str());
    }

    std::string extension = config["extension"];
    if (!extension.empty() && sys->loadExpansion(extension)) {
        printf("[INFO] Using extension %s\n", getFilenameExt(extension).c_str());
    }

    std::string iso = config["iso"];
    if (!iso.empty()) {
        loadFile(sys, iso);
        printf("Using iso %s\n", iso.c_str());
    }
}

// Warning: this method might have 1 or more miliseconds of inaccuracy.
void limitFramerate(SDL_Window* window, bool framelimiter) {
    static double counterFrequency = SDL_GetPerformanceFrequency();
    static double startTime = SDL_GetPerformanceCounter() / counterFrequency;
    static double fps;
    static double fpsTime = 0.0;
    static int deltaFrames = 0;

    double currentTime = SDL_GetPerformanceCounter() / counterFrequency;
    double deltaTime = currentTime - startTime;

    if (framelimiter) {
        while (deltaTime < 1.0 / 60.0) {  // calculate real difference
            SDL_Delay(1);

            currentTime = SDL_GetPerformanceCounter() / counterFrequency;
            deltaTime = currentTime - startTime;
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
        std::string gameName;
        if (sys->cdrom->cue.file.empty())
            gameName = "No CD";
        else
            gameName = getFilename(sys->cdrom->cue.file);

        std::string title = string_format("Avocado %s | %s | FPS: %.0f (%0.2f ms) %s", BUILD_STRING, gameName.c_str(), fps,
                                          (1.0 / fps) * 1000.0, !framelimiter ? "unlimited" : "");
        SDL_SetWindowTitle(window, title.c_str());
    }
}

int main(int argc, char** argv) {
    loadConfigFile(CONFIG_NAME);

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER | SDL_INIT_JOYSTICK | SDL_INIT_AUDIO) != 0) {
        printf("Cannot init SDL\n");
        return 1;
    }

    OpenGL opengl;
    if (!opengl.init()) {
        printf("Cannot initialize OpenGL\n");
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow("Avocado", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, OpenGL::resWidth, OpenGL::resHeight,
                                          SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL);
    if (window == nullptr) {
        printf("Cannot create window (%s)\n", SDL_GetError());
        return 1;
    }

    SDL_GLContext glContext = SDL_GL_CreateContext(window);
    if (glContext == nullptr) {
        printf("Cannot create OpenGL context\n");
        return 1;
    }

    if (!opengl.setup()) {
        printf("Cannot setup graphics\n");
        return 1;
    }

    AudioCD::init();

    hardReset();

    SDL_EventState(SDL_DROPFILE, SDL_ENABLE);
    SDL_GameControllerEventState(SDL_ENABLE);

    ImGui::CreateContext();
    ImGui_ImplSdlGL3_Init(window);
    ImGui::StyleColorsDark();

    SDL_GL_SetSwapInterval(0);

    vramTextureId = opengl.getVramTextureId();
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
        while (newEvent || SDL_PollEvent(&event)) {
            newEvent = false;
            if (event.type == SDL_QUIT || (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE)) running = false;
            if (event.type == SDL_WINDOWEVENT) {
                if (event.window.event == SDL_WINDOWEVENT_FOCUS_LOST) windowFocused = false;
                if (event.window.event == SDL_WINDOWEVENT_FOCUS_GAINED) windowFocused = true;
            }
            if (event.type == SDL_KEYDOWN && event.key.repeat == 0) {
                if (waitingForKeyPress) {
                    waitingForKeyPress = false;
                    if (event.key.keysym.sym != SDLK_ESCAPE) lastPressedKey = event.key.keysym.sym;
                    continue;
                }
                if (event.key.keysym.sym == SDLK_ESCAPE) running = false;
                if (event.key.keysym.sym == SDLK_2) sys->interrupt->trigger(interrupt::TIMER2);
                if (event.key.keysym.sym == SDLK_d) sys->interrupt->trigger(interrupt::DMA);
                if (event.key.keysym.sym == SDLK_s) sys->interrupt->trigger(interrupt::SPU);
                if (event.key.keysym.sym == SDLK_r) {
                    sys->dumpRam();
                    sys->spu->dumpRam();
                }
                if (event.key.keysym.sym == SDLK_F1) showGui = !showGui;
                if (event.key.keysym.sym == SDLK_F2) {
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
                    if (sys->state == System::State::pause)
                        sys->state = System::State::run;
                    else if (sys->state == System::State::run)
                        sys->state = System::State::pause;
                }
                if (event.key.keysym.sym == SDLK_q) {
                    showVRAM = !showVRAM;
                }
                if (event.key.keysym.sym == SDLK_TAB) frameLimitEnabled = !frameLimitEnabled;
            }
            if (event.type == SDL_DROPFILE) {
                std::string path = event.drop.file;
                SDL_free(event.drop.file);
                loadFile(sys, path);
            }
            if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                opengl.width = event.window.data1;
                opengl.height = event.window.data2;
            }
            sys->controller->setState(getButtonState(event));
            ImGui_ImplSdlGL3_ProcessEvent(&event);
        }

        if (showVRAM != opengl.getViewFullVram()) {
            if (showVRAM)
                SDL_SetWindowSize(window, 1024, 512);
            else
                SDL_SetWindowSize(window, OpenGL::resWidth, OpenGL::resHeight);
            opengl.setViewFullVram(showVRAM);
        }

        if (doHardReset) {
            doHardReset = false;
            hardReset();
        }

        if (sys->state == System::State::run) {
            sys->emulateFrame();
            if (singleFrame) {
                singleFrame = false;
                sys->state = System::State::pause;
            }
        }
        ImGui_ImplSdlGL3_NewFrame(window);

        opengl.render(sys->gpu.get());
        renderImgui(sys.get());

        SDL_GL_SwapWindow(window);

        limitFramerate(window, frameLimitEnabled);
    }
    saveConfigFile(CONFIG_NAME);

    AudioCD::close();
    ImGui_ImplSdlGL3_Shutdown();
    ImGui::DestroyContext();
    SDL_GL_DeleteContext(glContext);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}