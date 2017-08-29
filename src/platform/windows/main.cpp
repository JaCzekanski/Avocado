#include <cstdio>
#include <string>
#include <algorithm>
#include <SDL.h>
#include "renderer/opengl/opengl.h"
#include "utils/string.h"
#include "mips.h"
#include "imgui/imgui_impl_sdl_gl3.h"
#include "gui.h"
#include "utils/cue/cueParser.h"

#undef main

const int CPU_CLOCK = 33868500;
const int GPU_CLOCK_NTSC = 53690000;

device::controller::DigitalController &getButtonState(SDL_Event &event) {
    static device::controller::DigitalController buttons;
    if (event.type == SDL_KEYDOWN) {
        if (event.key.keysym.sym == SDLK_UP) buttons.up = true;
        if (event.key.keysym.sym == SDLK_DOWN) buttons.down = true;
        if (event.key.keysym.sym == SDLK_LEFT) buttons.left = true;
        if (event.key.keysym.sym == SDLK_RIGHT) buttons.right = true;
        if (event.key.keysym.sym == SDLK_KP_2) buttons.cross = true;
        if (event.key.keysym.sym == SDLK_KP_8) buttons.triangle = true;
        if (event.key.keysym.sym == SDLK_KP_4) buttons.square = true;
        if (event.key.keysym.sym == SDLK_KP_6) buttons.circle = true;
        if (event.key.keysym.sym == SDLK_KP_3) buttons.start = true;
        if (event.key.keysym.sym == SDLK_KP_1) buttons.select = true;
        if (event.key.keysym.sym == SDLK_KP_7) buttons.l1 = true;
        if (event.key.keysym.sym == SDLK_KP_DIVIDE) buttons.l2 = true;
        if (event.key.keysym.sym == SDLK_KP_9) buttons.r1 = true;
        if (event.key.keysym.sym == SDLK_KP_MULTIPLY) buttons.r2 = true;
    }
    if (event.type == SDL_KEYUP) {
        if (event.key.keysym.sym == SDLK_UP) buttons.up = false;
        if (event.key.keysym.sym == SDLK_DOWN) buttons.down = false;
        if (event.key.keysym.sym == SDLK_LEFT) buttons.left = false;
        if (event.key.keysym.sym == SDLK_RIGHT) buttons.right = false;
        if (event.key.keysym.sym == SDLK_KP_2) buttons.cross = false;
        if (event.key.keysym.sym == SDLK_KP_8) buttons.triangle = false;
        if (event.key.keysym.sym == SDLK_KP_4) buttons.square = false;
        if (event.key.keysym.sym == SDLK_KP_6) buttons.circle = false;
        if (event.key.keysym.sym == SDLK_KP_3) buttons.start = false;
        if (event.key.keysym.sym == SDLK_KP_1) buttons.select = false;
        if (event.key.keysym.sym == SDLK_KP_7) buttons.l1 = false;
        if (event.key.keysym.sym == SDLK_KP_DIVIDE) buttons.l2 = false;
        if (event.key.keysym.sym == SDLK_KP_9) buttons.r1 = false;
        if (event.key.keysym.sym == SDLK_KP_MULTIPLY) buttons.r2 = false;
    }

    return buttons;
}

bool running = true;
int start(int argc, char **argv) {
    std::string bios = "SCPH1001.bin";
    std::string iso = "D:/Games/!PSX/Doom/Doom (Track 1).bin";

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

    SDL_EventState(SDL_DROPFILE, SDL_ENABLE);

    ImGui_ImplSdlGL3_Init(window);

    std::unique_ptr<mips::CPU> cpu = std::make_unique<mips::CPU>();

    if (cpu->loadBios(bios)) {
        printf("Using bios %s\n", bios.c_str());
    }
    //        cpu->loadExpansion("data/bios/expansion.rom");

    cpu->cdrom->setShell(true);  // open shell
    if (fileExists(iso)) {
        bool success = cpu->dma->dma3.load(iso);
        cpu->cdrom->setShell(!success);
        if (!success) printf("Cannot load iso file: %s\n", iso.c_str());
    }

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
                if (event.key.keysym.sym == SDLK_F5) {
                    cpu->biosLog = !cpu->biosLog;
                }
                if (event.key.keysym.sym == SDLK_F6) {
                    cpu->disassemblyEnabled = !cpu->disassemblyEnabled;
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
                std::string ext = getExtension(path);
                std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                SDL_free(event.drop.file);

                if (ext == "iso" || ext == "bin" || ext == "img") {
                    if (fileExists(path)) {
                        printf("Dropped .iso, loading... ");

                        bool success = cpu->dma->dma3.load(path);
                        cpu->cdrom->setShell(!success);

                        if (success)
                            printf("ok.\n");
                        else
                            printf("fail.\n");
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

        std::string title = string_format("Avocado: IMASK: %s, ISTAT: %s, frame: %d, FPS: %.0f", cpu->interrupt->getMask().c_str(),
                                          cpu->interrupt->getStatus().c_str(), cpu->getGPU()->frames, fps);
        SDL_SetWindowTitle(window, title.c_str());
        SDL_GL_SwapWindow(window);
    }
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