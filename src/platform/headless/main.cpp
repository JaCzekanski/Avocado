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

    if (!cpu->loadExpansion("data/asm/bootstrap.bin")) {
        printf("cannot load expansion!\n");
        return 1;
    }

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

    cpu->state = mips::CPU::State::run;
    cpu->debugOutput = false;
    cpu->setPC(cpu->readMemory32(0x1f000000));

    while (cpu->state == mips::CPU::State::run) {
        cpu->emulateFrame();
    }

    return 0;
}
