#include <cstdio>
#include <string>
#include <memory>
#include "utils/file.h"
#include "mips.h"

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
    std::unique_ptr<mips::CPU> cpu = std::make_unique<mips::CPU>();

    auto _bios = getFileContents("data/bios/SCPH1001.BIN");  // DTLH3000.BIN BOOTS
    if (_bios.empty()) {
        printf("Cannot open BIOS");
        return 1;
    }

    assert(_bios.size() == 512 * 1024);
    std::copy(_bios.begin(), _bios.end(), cpu->bios);
    cpu->state = mips::CPU::State::run;

    auto gpu = std::make_unique<device::gpu::GPU>();
    cpu->setGPU(gpu.get());

    for (;;) {
        if (cpu->state != mips::CPU::State::run) break;

        emulateFrame(cpu, gpu);
    }
    return 0;
}
