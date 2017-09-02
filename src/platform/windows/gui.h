#pragma once
#include "mips.h"

void renderImgui(mips::CPU *cpu);
extern int vramTextureId;
extern bool gteRegistersEnabled;
extern bool ioLogEnabled;
extern bool gteLogEnabled;
extern bool gpuLogEnabled;
extern bool showVRAM;
extern bool singleFrame;
extern bool skipRender;
extern bool showIo;
extern bool exitProgram;
extern bool doHardReset;