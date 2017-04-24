#include <cstdio>
#include <string>
#include <memory>
#include <cassert>
#include "mips.h"

int main(int argc, char **argv) {
	if (argc < 2) {
		printf("usage: Avocado psx.exe");
		return 1;
	}
    std::unique_ptr<mips::CPU> cpu = std::make_unique<mips::CPU>();
	auto gpu = std::make_unique<device::gpu::GPU>();
    cpu->setGPU(gpu.get());

	if (!cpu->loadBios("SCPH1001.BIN")) {
        return 1;
    }

	if (!cpu->loadExpansion("data/asm/bootstrap.bin")) {
		return 1;
	}

	cpu->biosLog = false;

    while (cpu->state == mips::CPU::State::run) {
        cpu->emulateFrame();
    }

    if (!cpu->loadExeFile(argv[1])) {
		printf("Cannot load %s\n", argv[1]);
		return 1;
   	}

	cpu->state = mips::CPU::State::run;
	cpu->PC+=4;
    while (cpu->state == mips::CPU::State::run) {
        cpu->emulateFrame();
    }

    return 0;
}
