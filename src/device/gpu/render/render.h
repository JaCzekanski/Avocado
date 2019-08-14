#pragma once
#include "device/gpu/gpu.h"

class Render {
   public:
    static void drawLine(gpu::GPU* gpu, const int16_t x[2], const int16_t y[2], const RGB c[2]);
    static void drawTriangle(gpu::GPU* gpu, gpu::Vertex v[3]);
    static void drawRectangle(gpu::GPU* gpu, const Rectangle& rect);
};