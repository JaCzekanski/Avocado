#include <cstdio>
#include <string>
#include <SDL.h>
#include "utils/file.h"
#include "utils/string.h"
#include "mips.h"

#undef main

bool disassemblyEnabled = false;
char *_mnemonic;
std::string _disasm = "";

mips::CPU cpu;
bool cpuRunning = true;

const int cpuFrequency = 44100 * 768;
const int gpuFrequency = cpuFrequency * 11 / 7;

void render(SDL_Renderer* renderer, device::gpu::GPU* gpu)
{
	gpu->render();

	SDL_Rect dst = { 0, 0, 320, 240 };
	SDL_RenderCopy(renderer, gpu->output, NULL, &dst);

	dst = { 320, 0, 320, 240 };
	SDL_RenderCopy(renderer, gpu->texture, NULL, &dst);
	SDL_RenderPresent(renderer);
}

int main(int argc, char **argv) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        printf("Cannot init SDL\n");
        return 1;
    }
    SDL_Window *window;
    SDL_Renderer *renderer;

	window = SDL_CreateWindow("Avocado", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 640, 480, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
	if (window == nullptr)
	{
		printf("Cannot create window\n");
		return 1;
	}

	SDL_EventState(SDL_DROPFILE, SDL_ENABLE);

	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
	if (renderer == nullptr) {
        printf("Cannot create renderer\n");
        return 1;
    }

    auto _bios = getFileContents("data/bios/SCPH1001.BIN");
    if (_bios.empty()) {
        printf("Cannot open BIOS");
        return 1;
    }
    memcpy(cpu.bios, &_bios[0], _bios.size());

	auto _exp = getFileContents("data/bios/expansion.rom");
	if (!_exp.empty()) {
		memcpy(cpu.expansion, &_exp[0], _exp.size());
	}

    device::gpu::GPU *gpu = new device::gpu::GPU();
    cpu.setGPU(gpu);

    int gpuLine = 0;
    int gpuDot = 0;
    bool gpuOdd = false;

    int cycles = 0;
    int frames = 0;

	bool emulatorRunning = true;

    gpu->setRenderer(renderer);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0xff);
    SDL_RenderClear(renderer);

    SDL_Event event;

	device::controller::DigitalController buttons;

    while (emulatorRunning) {
		int pendingEvents = 0;
		if (!cpuRunning) SDL_WaitEvent(&event);
		else pendingEvents = SDL_PollEvent(&event);
        
		if (event.type == SDL_QUIT || (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE)) emulatorRunning = false;
        if (event.type == SDL_KEYDOWN) {
            if (event.key.keysym.sym == SDLK_SPACE) {
				cpu.memoryAccessLogging = !cpu.memoryAccessLogging;
                printf("MAL %d\n", cpu.memoryAccessLogging);
            }
            if (event.key.keysym.sym == SDLK_l) {
				std::string exePath = "data/exe/";
				char filename[128];
				printf("\nEnter exe name: ");
				scanf("%s", filename);
				exePath += filename;
				cpu.loadExeFile(exePath);
			}
            if (event.key.keysym.sym == SDLK_b) cpu.biosLog = !cpu.biosLog;
			if (event.key.keysym.sym == SDLK_c) cpu.interrupt->IRQ(2);
			if (event.key.keysym.sym == SDLK_f) cpu.cop0.status.interruptEnable = true;
			if (event.key.keysym.sym == SDLK_ESCAPE) emulatorRunning = false;

			if (event.key.keysym.sym == SDLK_UP) buttons.up = true;
			if (event.key.keysym.sym == SDLK_DOWN) buttons.down = true;
			if (event.key.keysym.sym == SDLK_LEFT) buttons.left = true;
			if (event.key.keysym.sym == SDLK_RIGHT) buttons.right = true;
			cpu.controller->setState(buttons);
        }
		if (event.type == SDL_KEYUP) {
			if (event.key.keysym.sym == SDLK_UP) buttons.up = false;
			if (event.key.keysym.sym == SDLK_DOWN) buttons.down = false;
			if (event.key.keysym.sym == SDLK_LEFT) buttons.left = false;
			if (event.key.keysym.sym == SDLK_RIGHT) buttons.right = false;
			cpu.controller->setState(buttons);
		}
		if (event.type == SDL_DROPFILE)
		{
			// TODO: SDL_free fails after few times
			cpu.loadExeFile(event.drop.file);
			SDL_free(event.drop.file);
		}
		if (pendingEvents) continue;
		if (!cpuRunning) continue;

		if (!cpu.executeInstructions(7)) {
            printf("CPU Halted\n");
            cpuRunning = false;
        }
		cycles += 7;

		for (int i = 0; i < 11; i++) {
			cpu.timer0->step();
			cpu.timer1->step();
			cpu.timer2->step();
            gpuDot++;

			if (gpuDot < 3412) continue;

            gpuDot = 0;
            gpuLine++;
			if (gpuLine == 0x100) {
				gpu->odd = false;
				gpu->step();
				cpu.interrupt->IRQ(0);
				continue;
            }
			if (gpuLine >= 263) {
                gpuLine = 0;
                frames++;
                gpuOdd = !gpuOdd;
                gpu->step();

                std::string title = string_format("IMASK: %s, ISTAT: %s, frame: %d,", cpu.interrupt->getMask().c_str(), cpu.interrupt->getStatus().c_str(), frames);
                SDL_SetWindowTitle(window, title.c_str());

				render(renderer, gpu);
            }
			else if (gpuLine > 0x10) {
				gpu->odd = gpuOdd;
			}
        }
    }

    SDL_Quit();
    return 0;
}
