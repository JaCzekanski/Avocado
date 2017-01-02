#include <cstdio>
#include <string>
#include <SDL.h>
#include "opengl/opengl.h"
#include "utils/file.h"
#include "utils/string.h"
#include "mips.h"
#include "gdbStub.h"

#undef main

bool disassemblyEnabled = false;
char *_mnemonic;
std::string _disasm = "";

mips::CPU cpu;
device::gpu::GPU *gpu;
bool cpuRunning = true;

const int cpuFrequency = 44100 * 768;
const int gpuFrequency = cpuFrequency * 11 / 7;

SDL_Window *window;
bool viewFullVram = false;

void dumpRam(mips::CPU *cpu) {
    std::vector<uint8_t> ram(cpu->ram, &cpu->ram[0x200000 - 1]);
    putFileContents("ram.bin", ram);
}

int gpuLine = 0;
int gpuDot = 0;
bool gpuOdd = false;
int frames = 0;

void emulateGpuCycles(int cycles) {
    // for (int i = 0; i<cycles; i++) cpu.timer0->step();
    // for (int i = 0; i<cycles; i++) cpu.timer1->step();
    // for (int i = 0; i<cycles; i++) cpu.timer2->step();

    gpuDot += cycles;

    int newLines = gpuDot / 3413;
    if (newLines == 0) return;

    cpu.timer1->step();

    gpuDot %= 3413;
    if (gpuLine < 0x100 && gpuLine + newLines >= 0x100) {
        gpu->odd = false;
        gpu->step();
        cpu.interrupt->IRQ(0);
    }
    gpuLine += newLines;
    if (gpuLine >= 263) {
        gpuLine %= 263;
        frames++;
        gpuOdd = !gpuOdd;
        gpu->step();

#ifndef HEADLESS
        std::string title = string_format("IMASK: %s, ISTAT: %s, frame: %d,", cpu.interrupt->getMask().c_str(),
                                          cpu.interrupt->getStatus().c_str(), frames);
        SDL_SetWindowTitle(window, title.c_str());

        opengl::render(gpu);
        SDL_GL_SwapWindow(window);
#endif
    } else if (gpuLine > 0x10) {
        gpu->odd = gpuOdd;
    }
}

struct EvCB {
    uint32_t clazz;
    uint32_t status;
    uint32_t spec;
    uint32_t mode;
    uint32_t ptr;
    uint32_t unk1;
    uint32_t unk2;
};

int main(int argc, char **argv) {
#ifndef HEADLESS
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        printf("Cannot init SDL\n");
        return 1;
    }
    opengl::init();

    //	GdbStub gdbStub;
    //	if (!gdbStub.initialize()) {
    //		printf("Cannot initialize networking");
    //		return 1;
    //	}

    window = SDL_CreateWindow("Avocado", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, opengl::resWidth, opengl::resHeight,
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

    if (!opengl::loadExtensions()) {
        printf("Cannot initialize glad\n");
        return 1;
    }

    if (!opengl::setup()) {
        printf("Cannot setup graphics\n");
        return 1;
    }
    SDL_EventState(SDL_DROPFILE, SDL_ENABLE);
#endif

    auto _bios = getFileContents("data/bios/SCPH1001.BIN");  // DTLH3000.BIN BOOTS
    if (_bios.empty()) {
        printf("Cannot open BIOS");
        return 1;
    }
    memcpy(cpu.bios, &_bios[0], _bios.size());

    auto _exp = getFileContents("data/bios/expansion.rom");
    if (!_exp.empty()) {
        memcpy(cpu.expansion, &_exp[0], _exp.size());
    }

    gpu = new device::gpu::GPU();
    cpu.setGPU(gpu);
    cpu.state = mips::CPU::State::run;

    int cycles = 0;
    bool emulatorRunning = true;
    SDL_Event event;
    device::controller::DigitalController buttons;

    while (emulatorRunning) {
        int pendingEvents = 0;
// if (!cpuRunning) SDL_WaitEvent(&event);
// else pendingEvents = SDL_PollEvent(&event);
#ifndef HEADLESS
        pendingEvents = SDL_PollEvent(&event);
#endif

        if (event.type == SDL_QUIT || (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE))
            emulatorRunning = false;
        if (event.type == SDL_KEYDOWN) {
            if (event.key.keysym.sym == SDLK_SPACE) {
                disassemblyEnabled = true;
            }
            if (event.key.keysym.sym == SDLK_l) {
                std::string exePath = "data/exe/";
                char filename[128];
                printf("\nEnter exe name: ");
                scanf("%s", filename);
                exePath += filename;
                cpu.loadExeFile(exePath);
            }
            if (event.key.keysym.sym == SDLK_e) {
                // Print EvCB
                uint32_t addr = cpu.readMemory32(0x120);
                uint32_t size = cpu.readMemory32(0x120 + 4) / 0x1c;

                EvCB *eventArray = (EvCB *)&cpu.ram[addr & 0x1FFFF];

                for (int i = 0; i < size; i++) {
                    if (eventArray[i].clazz == 0) break;
                    printf("Event %d  0x%08x 0x%08x 0x%08x\n", i, eventArray[i].clazz, eventArray[i].spec, eventArray[i].status);
                }
            }
            if (event.key.keysym.sym == SDLK_b) cpu.biosLog = !cpu.biosLog;
            if (event.key.keysym.sym == SDLK_c) cpu.interrupt->IRQ(2);
            if (event.key.keysym.sym == SDLK_d) cpu.interrupt->IRQ(3);
            if (event.key.keysym.sym == SDLK_f) cpu.cop0.status.interruptEnable = true;
            if (event.key.keysym.sym == SDLK_r) dumpRam(&cpu);
            if (event.key.keysym.sym == SDLK_q) {
                viewFullVram = !viewFullVram;

                if (viewFullVram)
                    SDL_SetWindowSize(window, 1024, 512);
                else
                    SDL_SetWindowSize(window, opengl::resWidth, opengl::resHeight);
            }
            //			if (event.key.keysym.sym == SDLK_b) gdbStub.sendBreak = true;
            if (event.key.keysym.sym == SDLK_ESCAPE) emulatorRunning = false;

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

            cpu.controller->setState(buttons);
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
            cpu.controller->setState(buttons);
        }
        if (event.type == SDL_DROPFILE) {
            // TODO: SDL_free fails after few times
            cpu.loadExeFile(event.drop.file);
            SDL_free(event.drop.file);
        }
        // gdbStub.handle(cpu);
        if (cpu.state != mips::CPU::State::run) SDL_Delay(10);
        if (pendingEvents) continue;

        if (!cpuRunning) {
            if (cpu.state == mips::CPU::State::run) cpuRunning = true;
            continue;
        }

        cpu.cdrom->step();

        if (!cpu.executeInstructions(70)) {
            printf("CPU Halted\n");
            cpuRunning = false;
        }
        cycles += 700;

        emulateGpuCycles(110);
    }
// gdbStub.uninitialize();
#ifndef HEADLESS
    SDL_GL_DeleteContext(glContext);
    SDL_DestroyWindow(window);
    SDL_Quit();
#endif
    return 0;
}
