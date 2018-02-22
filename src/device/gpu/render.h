#pragma once
#include "gpu.h"

void drawLine(GPU* gpu, const int16_t x[2], const int16_t y[2], const RGB c[2]);
void drawTriangle(GPU* gpu, Vertex v[3]);
void drawRectangle(GPU* gpu, const int16_t x[4], const int16_t y[4], const RGB color[4], const TextureInfo tex, bool textured, int flags);