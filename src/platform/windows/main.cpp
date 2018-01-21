#include <cstdio>
#include <string>
#include <algorithm>
#include <SDL.h>
#include <json.hpp>
#include "renderer/opengl/opengl.h"
#include "utils/string.h"
#include "mips.h"
#include "imgui/imgui_impl_sdl_gl3.h"
#include "platform/windows/gui/gui.h"
#include "utils/cue/cueParser.h"
#include "utils/file.h"
#include "platform/windows/config.h"
#include "utils/profiler/profiler.h"

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

void loadFile(std::unique_ptr<mips::CPU>& cpu, std::string path) {
    std::string ext = getExtension(path);
    transform(ext.begin(), ext.end(), ext.begin(), tolower);

    if (ext == "exe" || ext == "psexe") {
        printf("Loading .exe is currently not supported.\n");
        return;
    }

    if (ext == "json") {
        auto file = getFileContents(path);
        if (file.empty()) {
            return;
        }
        nlohmann::json j = nlohmann::json::parse(file);

        auto& gpuLog = cpu->gpu.get()->gpuLogList;
        gpuLog.clear();
        for (int i = 0; i < j.size(); i++) {
            device::gpu::GPU::GPU_LOG_ENTRY e;
            e.command = j[i]["command"];
            e.cmd = (device::gpu::Command)(int)j[i]["cmd"];

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
        cpu->cdrom->setCue(cue);
        bool success = cpu->dma->dma3.load(cue->tracks[0].filename);
        cpu->cdrom->setShell(!success);
        printf("File %s loaded\n", getFilenameExt(path).c_str());
    }
}

std::unique_ptr<mips::CPU> cpu;

void hardReset() {
    cpu = std::make_unique<mips::CPU>();

    std::string bios = config["bios"];
    if (!bios.empty() && cpu->loadBios(bios)) {
        printf("Using bios %s\n", bios.c_str());
    }

    std::string extension = config["extension"];
    if (!extension.empty() && cpu->loadExpansion(extension)) {
        printf("Using extension %s\n", extension.c_str());
    }

    std::string iso = config["iso"];
    if (!iso.empty()) {
        loadFile(cpu, iso);
        printf("Using iso %s\n", iso.c_str());
    }
}

int start(int argc, char** argv) {
    loadConfigFile(CONFIG_NAME);

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER | SDL_INIT_JOYSTICK) != 0) {
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

    hardReset();

    SDL_EventState(SDL_DROPFILE, SDL_ENABLE);
    SDL_GameControllerEventState(SDL_ENABLE);

    ImGui_ImplSdlGL3_Init(window);

    SDL_GL_SetSwapInterval(0);

    vramTextureId = opengl.getVramTextureId();
    if (!isEmulatorConfigured())
        cpu->state = mips::CPU::State::stop;
    else
        cpu->state = mips::CPU::State::run;

    float startTime = SDL_GetTicks() / 1000.f;
    float fps = 0.f;
    int deltaFrames = 0;

    Profiler::enable();

    SDL_Event event;
    while (running && !exitProgram) {
        bool newEvent = false;
        if (cpu->state != mips::CPU::State::run) {
            SDL_WaitEvent(&event);
            newEvent = true;
        }
        if (cpu->state != mips::CPU::State::pause) {
            Profiler::reset();
            Profiler::enable();
        } else {
            Profiler::disable();
        }

        Profiler::start("events");
        while (newEvent || SDL_PollEvent(&event)) {
            newEvent = false;
            if (event.type == SDL_QUIT || (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE)) running = false;
            if (event.type == SDL_KEYDOWN && event.key.repeat == 0) {
                if (waitingForKeyPress) {
                    waitingForKeyPress = false;
                    if (event.key.keysym.sym != SDLK_ESCAPE) lastPressedKey = event.key.keysym.sym;
                    continue;
                }
                if (event.key.keysym.sym == SDLK_ESCAPE) running = false;
                if (event.key.keysym.sym == SDLK_2) cpu->interrupt->trigger(interrupt::TIMER2);
                if (event.key.keysym.sym == SDLK_c) cpu->interrupt->trigger(interrupt::CDROM);
                if (event.key.keysym.sym == SDLK_d) cpu->interrupt->trigger(interrupt::DMA);
                if (event.key.keysym.sym == SDLK_s) cpu->interrupt->trigger(interrupt::SPU);
                if (event.key.keysym.sym == SDLK_TAB) skipRender = !skipRender;
                if (event.key.keysym.sym == SDLK_r) {
                    cpu->dumpRam();
                    cpu->spu->dumpRam();
                }
                if (event.key.keysym.sym == SDLK_F2) {
                    cpu->softReset();
                }
                if (event.key.keysym.sym == SDLK_F3) {
                    printf("Shell toggle\n");
                    cpu->cdrom->toggleShell();
                }
                if (event.key.keysym.sym == SDLK_F7) {
                    singleFrame = true;
                    cpu->state = mips::CPU::State::run;
                }
                if (event.key.keysym.sym == SDLK_F8) {
                    cpu->singleStep();
                }
                if (event.key.keysym.sym == SDLK_SPACE) {
                    if (cpu->state == mips::CPU::State::pause)
                        cpu->state = mips::CPU::State::run;
                    else if (cpu->state == mips::CPU::State::run)
                        cpu->state = mips::CPU::State::pause;
                }
                if (event.key.keysym.sym == SDLK_q) {
                    showVRAM = !showVRAM;
                }
            }
            if (event.type == SDL_DROPFILE) {
                std::string path = event.drop.file;
                SDL_free(event.drop.file);
                loadFile(cpu, path);
            }
            if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                opengl.width = event.window.data1;
                opengl.height = event.window.data2;
            }
            cpu->controller->setState(getButtonState(event));
            ImGui_ImplSdlGL3_ProcessEvent(&event);
        }
        Profiler::stop("events");

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

        if (cpu->state == mips::CPU::State::run) {
            cpu->emulateFrame();
            if (singleFrame) {
                singleFrame = false;
                cpu->state = mips::CPU::State::pause;
            }
        }
        ImGui_ImplSdlGL3_NewFrame(window);

        cpu->gpu.get()->rasterize();
        if (!skipRender) opengl.render(cpu->gpu.get());

        cpu->gpu.get()->render().clear();

        renderImgui(cpu.get());

        deltaFrames++;
        float currentTime = SDL_GetTicks() / 1000.f;
        if (currentTime - startTime > 0.25f) {
            fps = (float)deltaFrames / (currentTime - startTime);
            startTime = currentTime;
            deltaFrames = 0;
        }

        std::string title
            = string_format("Avocado: IMASK: %s, ISTAT: %s, frame: %d, FPS: %.0f, ms: %0.2f", cpu->interrupt->getMask().c_str(),
                            cpu->interrupt->getStatus().c_str(), cpu->gpu.get()->frames, fps, (1.f / fps) * 1000.f);
        SDL_SetWindowTitle(window, title.c_str());
        SDL_GL_SwapWindow(window);
    }
    saveConfigFile(CONFIG_NAME);
    ImGui_ImplSdlGL3_Shutdown();
    SDL_GL_DeleteContext(glContext);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}

int main(int argc, char** argv) {
    int retval = start(argc, argv);
    if (retval != 0) {
        printf("\nPress enter to close");
        getchar();
    }
    return retval;
}
