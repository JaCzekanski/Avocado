#include "render.h"

#undef VRAM
#define VRAM ((uint16_t(*)[VRAM_WIDTH])gpu->vram.data())

void drawRectangle(GPU* gpu, const int16_t x[4], const int16_t y[4], const RGB color[4], const TextureInfo tex, bool textured, int flags) {}