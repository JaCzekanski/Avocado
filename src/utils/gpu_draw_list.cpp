#include "gpu_draw_list.h"
#include <cstdio>

namespace GpuDrawList {
int framesToCapture = 0;
int currentFrame = 0;

bool load(System *sys, const std::string &path) {
    FILE *f = fopen(path.c_str(), "rb");
    if (!f) {
        return false;
    }

    auto r32 = [f]() {
        uint32_t i;
        fread(&i, sizeof(i), 1, f);
        return i;
    };

    auto &log = sys->gpu->gpuLogList;
    log.clear();

    fread(sys->gpu->prevVram.data(), 2, sys->gpu->prevVram.size(), f);

    const int initialSetupCount = r32();
    for (int i = 0; i < initialSetupCount; i++) r32();

    const uint32_t commandCount = r32();
    for (int i = 0; i < commandCount; i++) {
        gpu::LogEntry entry;
        uint32_t header = r32();
        uint8_t type = (header >> 24) & 0xff;
        uint8_t count = header & 0xffffff;

        entry.type = type;
        for (int j = 0; j < count; j++) {
            entry.args.push_back(r32());
        }

        log.push_back(entry);
    }

    fclose(f);
    return true;
}

bool save(System *sys, const std::string &path) {
    FILE *f = fopen(path.c_str(), "wb");
    if (!f) {
        return false;
    }

    auto w32 = [f](uint32_t i) { fwrite(&i, sizeof(i), 1, f); };

    fwrite(sys->gpu->prevVram.data(), 2, sys->gpu->prevVram.size(), f);

    w32(0);  // No initial setup

    auto &log = sys->gpu->gpuLogList;
    w32(log.size());
    for (auto &entry : log) {
        w32((entry.type << 24) | entry.args.size());

        for (auto arg : entry.args) w32(arg);
    }

    fclose(f);
    return true;
}

void replayCommands(gpu::GPU *gpu, int to) {
    gpu->vram = gpu->prevVram;

    gpu->gpuLogEnabled = false;
    if (to == -1) to = gpu->gpuLogList.size() - 1;
    for (int i = 0; i <= to; i++) {
        auto entry = gpu->gpuLogList.at(i);

        if (entry.args.size() == 0) continue;
        if (entry.type == 0 && entry.cmd() == 0xc0) {
            continue;  // Skip Vram -> CPU
        }

        for (uint32_t arg : entry.args) {
            uint8_t addr = (entry.type == 0) ? 0 : 4;
            gpu->write(addr, arg);
        }
        gpu->emulateGpuCycles(100000);
    }
    gpu->gpuLogEnabled = true;
}

void dumpInitialState(gpu::GPU *gpu) {
    gpu->prevVram = gpu->vram;

    auto gp0 = [&](uint8_t cmd, uint32_t data) { gpu->gpuLogList.push_back(gpu::LogEntry::GP0(cmd, data)); };
    auto gp1 = [&](uint8_t cmd, uint32_t data) { gpu->gpuLogList.push_back(gpu::LogEntry::GP1(cmd, data)); };
    gp0(0xe1, gpu->gp0_e1._reg);
    gp0(0xe2, gpu->gp0_e2._reg);
    gp0(0xe3, ((gpu->drawingArea.top << 10) & 0xffc00) | (gpu->drawingArea.left & 0x3ff));
    gp0(0xe4, ((gpu->drawingArea.bottom << 10) & 0xffc00) | (gpu->drawingArea.right & 0x3ff));
    gp0(0xe5, ((gpu->drawingOffsetY & 0x7ff) << 11) | (gpu->drawingOffsetX & 0x7ff));
    gp0(0xe6, gpu->gp0_e6._reg);

    gp1(0x05, ((gpu->displayAreaStartY & 0x3ff) << 10) | (gpu->displayAreaStartX & 0x3ff));
    gp1(0x06, ((gpu->displayRangeX2 & 0xfff) << 12) | (gpu->displayRangeX1 & 0xfff));
    gp1(0x07, ((gpu->displayRangeY2 & 0x3ff) << 10) | (gpu->displayRangeY1 & 0x3ff));
    gp1(0x08, gpu->gp1_08._reg);
}
}  // namespace GpuDrawList