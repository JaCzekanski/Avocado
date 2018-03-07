#pragma once
#include <SDL.h>
#include "mips.h"

void renderImgui(mips::CPU *cpu);
extern bool showGui;
extern int vramTextureId;
extern bool gteRegistersEnabled;
extern bool ioLogEnabled;
extern bool gteLogEnabled;
extern bool gpuLogEnabled;
extern bool showVRAM;
extern bool singleFrame;
extern bool showIo;
extern bool exitProgram;
extern bool doHardReset;
extern bool waitingForKeyPress;
extern SDL_Keycode lastPressedKey;