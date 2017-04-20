#include <cstdio>
#include <string>
#include <SDL.h>
#include "renderer/opengl/opengl.h"
#include "utils/file.h"
#include "utils/string.h"
#include "mips.h"
#undef main

int _prevCycles = 0;
int _cycles = 0;

const int CPU_CLOCK = 33868500;
const int GPU_CLOCK_NTSC = 53690000;

bool emulateGpuCycles(std::unique_ptr<mips::CPU> &cpu, std::unique_ptr<device::gpu::GPU> &gpu, int cycles) {
    const int LINE_VBLANK_START_NTSC = 243;
    const int LINES_TOTAL_NTSC = 263;
    static int gpuLine = 0;
    static int gpuDot = 0;

    _cycles += cycles;
    gpuDot += cycles;

    int newLines = gpuDot / 3413;
    if (newLines == 0) return false;
    gpuDot %= 3413;
    gpuLine += newLines;

    if (gpuLine < LINE_VBLANK_START_NTSC - 1) {
        if (gpu->gp1_08.verticalResolution == device::gpu::GP1_08::VerticalResolution::r480 && gpu->gp1_08.interlace) {
            gpu->odd = gpu->frames % 2;
        } else {
            gpu->odd = gpuLine % 2;
        }
    } else {
        gpu->odd = false;
    }

    if (gpuLine == LINES_TOTAL_NTSC - 1) {
        gpuLine = 0;
        gpu->frames++;
        cpu->interrupt->IRQ(0);

        return true;
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
    int systemCycles = 991;
    for (;;) {
        if (!cpu->executeInstructions(systemCycles * 200 / 317)) {
            printf("CPU Halted\n");
            return;
        }

        cpu->cdrom->step();
        cpu->timer1->step(systemCycles);
        cpu->timer2->step(systemCycles);

        if (emulateGpuCycles(cpu, gpu, systemCycles)) {
            return;  // frame emulated
        }
    }
}

struct EvCB {
    uint32_t clazz;
    uint32_t status;
    uint32_t spec;
    uint32_t mode;
    uint32_t ptr;
    uint32_t _;
    uint32_t __;
};

void getEVCB(std::unique_ptr<mips::CPU> &cpu, bool ready) {
    uint32_t addr = cpu->readMemory32(0x120);
    uint32_t size = cpu->readMemory32(0x120 + 4);

    EvCB evcb;
    for (size_t n = 0; n < size / 0x1c; n++) {
        for (int i = 0; i < 0x1c; i++) {
            *((uint8_t *)&evcb + i) = cpu->readMemory8(addr + (n * 0x1c) + i);
        }
        if (evcb.clazz == 0) return;
        if (ready) evcb.status = 0x4000;
        for (int i = 0; i < 0x1c; i++) {
            cpu->writeMemory8(addr + (n * 0x1c) + i, *((uint8_t *)&evcb + i));
        }
        printf("EvCB %d\n", n);
        printf("class: 0x%08x\n", evcb.clazz);
        printf("status: 0x%08x\n", evcb.status);
        printf("spec: 0x%08x\n", evcb.spec);
        printf("mode: 0x%08x\n", evcb.mode);
        printf("ptr: 0x%08x\n\n", evcb.ptr);
    }
}

int main(int argc, char **argv) {
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

    std::unique_ptr<mips::CPU> cpu = std::make_unique<mips::CPU>();

    auto _bios = getFileContents("data/bios/DTLH3000.BIN");  // DTLH3000.BIN BOOTS
    if (_bios.empty()) {
        printf("Cannot open BIOS");
    } else {
        assert(_bios.size() == 512 * 1024);
        std::copy(_bios.begin(), _bios.end(), cpu->bios);
        cpu->state = mips::CPU::State::run;
    }

    auto _exp = getFileContents("data/bios/expansion.rom");
    if (!_exp.empty()) {
        assert(_exp.size() < cpu->EXPANSION_SIZE);
        std::copy(_exp.begin(), _exp.end(), cpu->expansion);
    }

    auto gpu = std::make_unique<device::gpu::GPU>();
    cpu->setGPU(gpu.get());

    SDL_Event event;

    for (;;) {
        int pendingEvents = SDL_PollEvent(&event);

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
            if (event.key.keysym.sym == SDLK_s) cpu->interrupt->IRQ(9);
            if (event.key.keysym.sym == SDLK_o) cpu->cdrom->shellOpen = !cpu->cdrom->shellOpen;
            if (event.key.keysym.sym == SDLK_f) cpu->cop0.status.interruptEnable = true;
            if (event.key.keysym.sym == SDLK_w) getEVCB(cpu, true);
            if (event.key.keysym.sym == SDLK_e) getEVCB(cpu, false);
            if (event.key.keysym.sym == SDLK_r) {
                cpu->dumpRam();
                cpu->spu->dumpRam();
            }
            if (event.key.keysym.sym == SDLK_p) {
                if (cpu->state == mips::CPU::State::pause)
                    cpu->state = mips::CPU::State::run;
                else if (cpu->state == mips::CPU::State::run)
                    cpu->state = mips::CPU::State::pause;
            }
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

            opengl.render(gpu.get());
            std::string title = string_format("Avocado: IMASK: %s, ISTAT: %s, frame: %d, CPU cycles: %d", cpu->interrupt->getMask().c_str(),
                                              cpu->interrupt->getStatus().c_str(), gpu->frames, (_cycles - _prevCycles) * 200 / 317);
            _prevCycles = _cycles;
            SDL_SetWindowTitle(window, title.c_str());
            SDL_GL_SwapWindow(window);
        } else {
            SDL_Delay(16);
        }
    }
    SDL_GL_DeleteContext(glContext);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
