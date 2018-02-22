#pragma once
#include "gpu.h"

void drawLine(GPU* gpu, const int16_t x[2], const int16_t y[2], const RGB c[2]);
void drawTriangle(GPU* gpu, Vertex v[3]);