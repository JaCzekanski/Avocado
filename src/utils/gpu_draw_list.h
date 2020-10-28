#pragma once
#include <string>
#include "system.h"

namespace GpuDrawList {
extern int framesToCapture;
extern int currentFrame;

bool load(System *sys, const std::string &path);
bool save(System *sys, const std::string &path);
void replayCommands(gpu::GPU *gpu, int to = -1);
void dumpInitialState(gpu::GPU *gpu);
}  // namespace GpuDrawList