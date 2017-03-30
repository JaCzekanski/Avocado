#include <cstdio>
#include <string>
#include <SDL.h>
#include "opengl/opengl.h"
#include "utils/file.h"
#include "utils/string.h"
#include "mips.h"
#undef main

bool emulateGpuCycles(std::unique_ptr<mips::CPU> &cpu, std::unique_ptr<device::gpu::GPU> &gpu, int cycles) {
    static int gpuLine = 0;
    static int gpuDot = 0;
    static bool gpuOdd = false;
    // for (int i = 0; i<cycles; i++) cpu->timer0->step();
    // for (int i = 0; i<cycles; i++) cpu->timer1->step();
    // for (int i = 0; i<cycles; i++) cpu->timer2->step();

    gpuDot += cycles;

    int newLines = gpuDot / 3413;
    if (newLines == 0) return false;

    cpu->timer1->step();

    gpuDot %= 3413;
    if (gpuLine < 0x100 && gpuLine + newLines >= 0x100) {
        gpu->odd = false;
        gpu->step();
        cpu->interrupt->IRQ(0);
    }
    gpuLine += newLines;
    if (gpuLine >= 263) {
        gpuLine %= 263;
        gpu->frames++;
        gpuOdd = !gpuOdd;
        gpu->step();

        return true;
    } else if (gpuLine > 0x10) {
        gpu->odd = gpuOdd;
    }
    return false;
}

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

void emulateFrame(std::unique_ptr<mips::CPU> &cpu, std::unique_ptr<device::gpu::GPU> &gpu) {
    for (;;) {
        cpu->cdrom->step();

        if (!cpu->executeInstructions(70)) {
            printf("CPU Halted\n");
            return;
        }
        if (emulateGpuCycles(cpu, gpu, 110)) {
            return;  // frame emulated
        }
    }
}

int main(int argc, char **argv) {
#ifndef HEADLESS
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
#endif

    std::unique_ptr<mips::CPU> cpu = std::make_unique<mips::CPU>();

    auto _bios = getFileContents("data/bios/SCPH1001.BIN");  // DTLH3000.BIN BOOTS
    if (_bios.empty()) {
        printf("Cannot open BIOS");
    } else {
        assert(_bios.size() == 512 * 1024);
        std::copy(_bios.begin(), _bios.end(), cpu->bios);
        cpu->state = mips::CPU::State::run;
    }

    auto _exp = getFileContents("data/bios/expansion.rom");
    if (!_exp.empty()) {
        assert(_exp.size() < 8192 * 1024);
        std::copy(_exp.begin(), _exp.end(), cpu->expansion);
    }

    auto gpu = std::make_unique<device::gpu::GPU>();
    cpu->setGPU(gpu.get());

    SDL_Event event;

    for (;;) {
        int pendingEvents = 0;
#ifndef HEADLESS
        pendingEvents = SDL_PollEvent(&event);
#endif

        if (event.type == SDL_QUIT || (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE)) break;
        if (event.type == SDL_KEYDOWN) {
            if (event.key.keysym.sym == SDLK_ESCAPE) break;
            if (event.key.keysym.sym == SDLK_SPACE) {
                cpu->disassemblyEnabled = !cpu->disassemblyEnabled;
            }
            if (event.key.keysym.sym == SDLK_l) {
                std::string exePath = "data/exe/";
                char filename[128];
                printf("\nEnter exe name: ");
                scanf("%s", filename);
                exePath += filename;
                cpu->loadExeFile(exePath);
            }
            if (event.key.keysym.sym == SDLK_b) cpu->biosLog = !cpu->biosLog;
            if (event.key.keysym.sym == SDLK_c) cpu->interrupt->IRQ(2);
            if (event.key.keysym.sym == SDLK_d) cpu->interrupt->IRQ(3);
            if (event.key.keysym.sym == SDLK_f) cpu->cop0.status.interruptEnable = true;
            if (event.key.keysym.sym == SDLK_r) cpu->dumpRam();
            if (event.key.keysym.sym == SDLK_q) {
                bool viewFullVram = !opengl.getViewFullVram();
                opengl.setViewFullVram(viewFullVram);
                if (viewFullVram)
                    SDL_SetWindowSize(window, 1024, 512);
                else
                    SDL_SetWindowSize(window, OpenGL::resWidth, OpenGL::resHeight);
            }
        }
        if (event.type == SDL_DROPFILE) {
            // TODO: SDL_free fails after few times
            cpu->loadExeFile(event.drop.file);
            SDL_free(event.drop.file);
        }
        cpu->controller->setState(getButtonState(event));
        if (pendingEvents) continue;

        if (cpu->state == mips::CPU::State::run) {
            emulateFrame(cpu, gpu);

#ifndef HEADLESS
            opengl.render(gpu.get());
#endif
            std::string title = string_format("Avocado: IMASK: %s, ISTAT: %s, frame: %d,", cpu->interrupt->getMask().c_str(),
                                              cpu->interrupt->getStatus().c_str(), gpu->frames);
            SDL_SetWindowTitle(window, title.c_str());
            SDL_GL_SwapWindow(window);
        } else {
            SDL_Delay(16);
        }
    }
#ifndef HEADLESS
    SDL_GL_DeleteContext(glContext);
    SDL_DestroyWindow(window);
    SDL_Quit();
#endif
    return 0;
}
