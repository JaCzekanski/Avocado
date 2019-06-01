#pragma once
#include <array>
#include <glm/glm.hpp>
#include <vector>
#include "psx_color.h"
#include "registers.h"

#define VRAM ((uint16_t(*)[VRAM_WIDTH])vram.data())

struct System;
class Render;
class OpenGL;

namespace gpu {

extern const char* CommandStr[];

const int VRAM_WIDTH = 1024;
const int VRAM_HEIGHT = 512;

const int LINE_VBLANK_START_NTSC = 243;
const int LINES_TOTAL_NTSC = 263;

class GPU {
    friend struct ::System;
    friend class ::Render;
    friend class ::OpenGL;

    int busToken;

    // TODO: Comment private fields
    int startX = 0;
    int startY = 0;
    int endX = 0;
    int endY = 0;
    int currX = 0;
    int currY = 0;
    int gpuReadMode = 0;
    uint32_t GPUREAD = 0;
    uint32_t GPUSTAT = 0;

    Command cmd = Command::None;
    uint8_t command = 0;
    std::array<uint32_t, MAX_ARGS> arguments{};  // TODO: MAX_ARGS + 1 ?
    int currentArgument = 0;
    int argumentCount = 0;

    int gpuLine = 0;
    int gpuDot = 0;

    bool odd = false;
    int frames = 0;

    // TODO: Move Debug GUI stuff to class and befriend it
   public:
    GP0_E1 gp0_e1;
    // GP0(0xe3)
    // GP0(0xe4)
    Rect<int16_t> drawingArea;

    // GP0(0xe5)
    int16_t drawingOffsetX = 0;
    int16_t drawingOffsetY = 0;

    // GP0(0xe6)
    GP0_E6 gp0_e6;

    // GP1(0x02)
    bool irqRequest;

    // GP1(0x03)
    bool displayDisable;

    // GP1(0x04)
    int dmaDirection = 0;

    // GP1(0x05)
    int16_t displayAreaStartX = 0;
    int16_t displayAreaStartY = 0;

    // GP1(0x06)
    int16_t displayRangeX1 = 0;
    int16_t displayRangeX2 = 0;

    // GP1(0x07)
    int16_t displayRangeY1 = 0;
    int16_t displayRangeY2 = 0;

    // GP1(0x09)
    bool textureDisableAllowed = false;

   private:
    // Hardware rendering
    std::vector<Vertex> vertices;

    bool softwareRendering;

    void reset();
    void cmdFillRectangle(uint8_t command);
    void cmdPolygon(PolygonArgs arg);
    void cmdLine(LineArgs arg);
    void cmdRectangle(RectangleArgs arg);
    void cmdCpuToVram1(uint8_t command);
    void cmdCpuToVram2(uint8_t command);
    void cmdVramToCpu(uint8_t command);
    void cmdVramToVram(uint8_t command);

    void drawPolygon(int16_t x[4], int16_t y[4], RGB c[4], TextureInfo t, bool isQuad = false, bool textured = false, int flags = 0);

    void writeGP0(uint32_t data);
    void writeGP1(uint32_t data);

    void reload();

   public:
    GP0_E2 gp0_e2;
    GP1_08 gp1_08;

    std::array<uint16_t, VRAM_WIDTH * VRAM_HEIGHT> vram{};

    GPU();
    ~GPU();
    void step();
    bool emulateGpuCycles(int cycles);
    uint32_t read(uint32_t address);
    void write(uint32_t address, uint32_t data);
    bool isNtsc();

    int minDrawingX(int x) const;
    int minDrawingY(int y) const;
    int maxDrawingX(int x) const;
    int maxDrawingY(int y) const;
    bool insideDrawingArea(int x, int y) const;

    bool gpuLogEnabled = true;
    std::vector<LogEntry> gpuLogList;
    std::array<uint16_t, VRAM_WIDTH * VRAM_HEIGHT> prevVram{};

    void clear() { vertices.clear(); }
};

}  // namespace gpu