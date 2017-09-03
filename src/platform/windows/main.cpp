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
#include "debugger/debugger.h"

#undef main

const int CPU_CLOCK = 33868500;
const int GPU_CLOCK_NTSC = 53690000;

device::controller::DigitalController &getButtonState(SDL_Event &event) {
    static device::controller::DigitalController buttons;
    for (auto it = config["controller"].begin(); it != config["controller"].end(); ++it) {
        auto button = it.key();
        auto keyName = it.value().get<std::string>();
        auto keyCode = SDL_GetKeyFromName(keyName.c_str());

        if (event.key.keysym.sym == keyCode) buttons.setByName(button, event.type == SDL_KEYDOWN);
    }

    return buttons;
}

bool running = true;

void loadFile(std::unique_ptr<mips::CPU> &cpu, std::string path) {
    std::string ext = getExtension(path);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    if (ext == "iso" || ext == "bin" || ext == "img") {
        if (fileExists(path)) {
            printf("Dropped .iso, loading... ");

            bool success = cpu->dma->dma3.load(path);
            cpu->cdrom->setShell(!success);

            if (success) {
                printf("ok.\n");
            } else {
                printf("fail.\n");
            }
        }
    } else if (ext == "exe" || ext == "psexe") {
        printf("Dropped .exe, currently not supported.\n");
    } else if (ext == "cue") {
        try {
            utils::CueParser parser;
            auto cue = parser.parse(path.c_str());
            cpu->cdrom->setCue(cue);
            bool success = cpu->dma->dma3.load(cue->tracks[0].filename);
            cpu->cdrom->setShell(!success);

            printf("File %s loaded\n", path.c_str());
        } catch (std::exception &e) {
            printf("Error parsing cue: %s\n", e.what());
        }
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

int start(int argc, char **argv) {
    loadConfigFile(CONFIG_NAME);

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        printf("Cannot init SDL\n");
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow("Avocado", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, OpenGL::resWidth, OpenGL::resHeight,
                                          SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL);
    if (window == nullptr) {
        printf("Cannot create window (%s)\n", SDL_GetError());
        return 1;
    }

    SDL_GLContext glContext = SDL_GL_CreateContext(window);
    if (glContext == nullptr) {
        printf("Cannot initialize opengl\n");
        return 1;
    }

    OpenGL opengl;

    if (!opengl.init()) {
        printf("Cannot initialize OpenGL\n");
        return 1;
    }

    if (!opengl.setup()) {
        printf("Cannot setup graphics\n");
        return 1;
    }

    hardReset();

    SDL_EventState(SDL_DROPFILE, SDL_ENABLE);
    ImGui_ImplSdlGL3_Init(window);

    vramTextureId = opengl.getVramTextureId();
    if (!isEmulatorConfigured()) cpu->state = mips::CPU::State::stop;

    float startTime = SDL_GetTicks() / 1000.f;
    float fps = 0.f;
    int deltaFrames = 0;

    SDL_Event event;
    while (running && !exitProgram) {
        bool newEvent = false;
        if (cpu->state != mips::CPU::State::run) {
            SDL_WaitEvent(&event);
            newEvent = true;
        }

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
                if (event.key.keysym.sym == SDLK_2) cpu->interrupt->trigger(device::interrupt::TIMER2);
                if (event.key.keysym.sym == SDLK_c) cpu->interrupt->trigger(device::interrupt::CDROM);
                if (event.key.keysym.sym == SDLK_d) cpu->interrupt->trigger(device::interrupt::DMA);
                if (event.key.keysym.sym == SDLK_s) cpu->interrupt->trigger(device::interrupt::SPU);
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
            cpu->controller->setState(getButtonState(event));
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

        if (cpu->state == mips::CPU::State::run) {
            cpu->emulateFrame();
            if (singleFrame) {
                singleFrame = false;
                cpu->state = mips::CPU::State::pause;
            }
        }
        ImGui_ImplSdlGL3_NewFrame(window);
        if (!skipRender)
            opengl.render(cpu->getGPU());
        else
            cpu->getGPU()->render().clear();
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
                            cpu->interrupt->getStatus().c_str(), cpu->getGPU()->frames, fps, (1.f / fps) * 1000.f);
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

int main(int argc, char **argv) {
    int retval = start(argc, argv);
    if (retval != 0) {
        printf("\nPress enter to close");
        getchar();
    }
    return retval;
}