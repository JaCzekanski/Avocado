#include <cstdio>
#include <string>
#include <SDL.h>
#include "utils/file.h"
#include "mips.h"
#include "psxExe.h"
#undef main

bool disassemblyEnabled = false;
bool memoryAccessLogging = false;
uint32_t memoryDumpAddress = 0;

char* _mnemonic;
std::string _disasm = "";

mips::CPU cpu;

const int cpuFrequency = 44100 * 768;
const int gpuFrequency = cpuFrequency * 11 / 7;

void IRQ(int irq)
{
	if (cpu.readMemory32(0x1f801074) & (1 << irq))
	{
		cpu.writeMemory32(0x1f801070, cpu.readMemory32(0x1f801070) | (1 << irq));
		cpu.COP0[14] = cpu.PC; // EPC - return address from trap
		cpu.COP0[13] |= (1 << 10); // Cause, IRQ
		cpu.COP0[12] |= (1 << 10) | 1;
		cpu.PC = 0x80000080;
		//printf("----IRQ%d\n", irq);
	}
}

bool loadExe = false;
int main( int argc, char** argv )
{
	if (SDL_Init(SDL_INIT_VIDEO) != 0) {
		printf("Cannot init SDL\n");
		return 1;
	}
	SDL_Window* window;
	SDL_Renderer* renderer;

	if (SDL_CreateWindowAndRenderer(640, 480, SDL_WINDOW_SHOWN, &window, &renderer) != 0) {
		printf("Cannot create window or renderer\n");
		return 1;
	}

	std::string biosPath = "data/bios/SCPH-102.BIN";
	auto _bios = getFileContents(biosPath);
	if (_bios.empty()) {
		printf("Cannot open BIOS");
		return 1;
	}
	memcpy(cpu.bios, &_bios[0], _bios.size());


	device::gpu::GPU *gpu = new device::gpu::GPU();
	cpu.setGPU(gpu);

	int gpuCycles = 0;
	int gpuLine = 0;
	int gpuDot = 0;
	bool vblank = false;

	int cycles = 0;
	int frames = 0;

	bool doDump = false;
	bool cpuRunning = true;

	gpu->setRenderer(renderer);
	SDL_SetRenderDrawColor(renderer, 0xff, 0xff, 0xff, 0xff);
	SDL_RenderClear(renderer);
	gpu->render();

	int dupa = 0;
	SDL_Event event;
	while (true)
	{
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT) break;
			if (event.type == SDL_KEYDOWN)
			{
				if (event.key.keysym.sym == SDLK_SPACE)
				{
					memoryAccessLogging = !memoryAccessLogging;
					printf("MAL %d\n", memoryAccessLogging);
				}
				if (event.key.keysym.sym == SDLK_i)	IRQ(0);
				if (event.key.keysym.sym == SDLK_l) loadExe = true;
			}
		}

		if (cpuRunning) {
			if (!cpu.executeInstructions(7)) {
				printf("CPU Halted\n");
				cpuRunning = false;
			}
			cycles += 7;

			for (int i = 0; i < 11; i++)
			{
				gpuCycles++;
				gpuDot++;

				if (gpuDot > 3413) {
					gpuDot = 0;

					gpuLine++;

					if (gpuLine == 240 && !vblank) {
						vblank = true;
						gpu->step();
						gpu->render(); 

						IRQ(0);
					}

					if (gpuLine > 263) {
						gpuLine = 0;
						gpuCycles = 0;
						vblank = false;

						frames++;
						gpu->odd = !gpu->odd;
					}
				}
			}
		}
		if (loadExe)
		{
			loadExe = false;

			PsxExe exe;
			std::string exePath = "data/exe/";
			char filename[128];
			printf("\nEnter exe name: ");
			scanf("%s", filename);
			exePath += filename;

			auto _exe = getFileContents(exePath);
			if (!_exe.empty()) {
				memcpy(&exe, &_exe[0], sizeof(exe));

				for (int i = 0x800; i < _exe.size(); i++) {
					cpu.writeMemory8(exe.t_addr + i - 0x800, _exe[i]);
				}

				cpu.PC = exe.pc0;
			}
		}
		if (doDump)
		{
			std::vector<uint8_t> ramdump;
			ramdump.resize(2 * 1024 * 1024);
			memcpy(&ramdump[0], cpu.ram, ramdump.size());
			putFileContents("ram.bin", ramdump);
			doDump = false;
		}
	}

	
	SDL_Quit();
	return 0;
}