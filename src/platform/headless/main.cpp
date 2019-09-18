#include <cassert>
#include <cstdio>
#include <memory>
#include <string>
#include "mips.h"
#include "utils/file.h"

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("usage: Avocado psx.exe\n");
        return 1;
    }
    std::unique_ptr<mips::CPU> cpu = std::make_unique<mips::CPU>();

    if (!cpu->loadBios("SCPH1001.BIN")) {
        return 1;
    }

    // Breakpoint on BIOS Shell execution
    cpu->breakpoints.emplace(0x80030000, mips::CPU::Breakpoint(true));

    cpu->biosLog = false;
    cpu->debugOutput = false;

    // Emulate BIOS to GUI breakpoint
    while (cpu->state == mips::CPU::State::run) {
        cpu->emulateFrame();
    }

    if (!cpu->loadExeFile(argv[1])) {
        printf("Cannot load %s\n", argv[1]);
        return 1;
    }
    printf("File %s loaded\n", getFilenameExt(argv[1]).c_str());

    cpu->state = System::State::run;
    cpu->debugOutput = false;
    cpu->setPC(cpu->readMemory32(0x1f000000));

    while (cpu->state == System::State::run) {
        cpu->emulateFrame();
    }

    return 0;
}
