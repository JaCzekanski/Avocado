#include <cstdio>
#include <string>
#include <memory>
#include <cassert>
#include "mips.h"

int main(int argc, char **argv) {
    std::unique_ptr<mips::CPU> cpu = std::make_unique<mips::CPU>();
	auto gpu = std::make_unique<device::gpu::GPU>();
    cpu->setGPU(gpu.get());

	if (!cpu->loadBios("SCPH1001.BIN")) {
        return 1;
    }

    for (;;) {
        if (cpu->state != mips::CPU::State::run) break;
        cpu->emulateFrame();
    }
    return 0;
}
