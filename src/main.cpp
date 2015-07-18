#include <cstdio>
#include <string>
#include <SDL.h>
#include "utils/file.h"
#include "mips.h"
#undef main

bool disassemblyEnabled = false;
bool memoryAccessLogging = false;

char* _mnemonic;
std::string _disasm = "";

mips::CPU cpu;

int main( int argc, char** argv )
{
	std::string biosPath = "data/bios/SCPH1000.bin";
	auto _bios = getFileContents(biosPath);
	if (_bios.empty()) {
		printf("Cannot open BIOS");
		return 1;
	}
	memcpy(cpu.bios, &_bios[0], _bios.size());

	int cycles = 0;
	int frames = 0;

	bool doDump = false;
	bool cpuRunning = true;
	while (cpuRunning)
	{
		if (!cpu.executeInstructions(1000)) {
			printf("CPU Halted\n");
			cpuRunning = false;
		}

		cycles += 1000*2;

		if (cycles >= 34000000.f/50.f) {
			cycles = 0;
			frames++;
			printf("Frame: %d\n", frames);
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

	return 0;
}