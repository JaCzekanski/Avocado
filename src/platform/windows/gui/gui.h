#pragma once
#include <SDL.h>
#include "file.h"
#include "platform/windows/input/key.h"
#include "system.h"

void initGui();
void deinitGui();
void renderImgui(System* sys);
extern bool showGui;
extern bool gteRegistersEnabled;
extern bool ioLogEnabled;
extern bool gteLogEnabled;
extern bool gpuLogEnabled;
extern bool singleFrame;
extern bool showIo;