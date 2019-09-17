#pragma once
#include <array>
#include <cereal/access.hpp>
#include <glm/glm.hpp>
#include <vector>
#include "primitive.h"
#include "psx_color.h"
#include "registers.h"

#define VRAM ((uint16_t(*)[VRAM_WIDTH])vram.data())

struct System;
class Render;
class OpenGL;

namespace gpu {

const int VRAM_WIDTH = 1024;
const int VRAM_HEIGHT = 512;

const int LINE_VBLANK_START_NTSC = 243;
const int LINES_TOTAL_NTSC = 263;

class GPU {
    friend class cereal::access;
    friend struct ::System;
    friend class ::Render;
    friend class ::OpenGL;

    int busToken;

    // Copy/Fill commands
    int startX = 0;
    int startY = 0;
    int endX = 0;
    int endY = 0;
    int currX = 0;
    int currY = 0;
    ReadMode readMode = ReadMode::Register;
    uint32_t readData = 0;

    // Command decoding
    Command cmd = Command::None;
    uint8_t command = 0;
    std::array<uint32_t, MAX_ARGS> arguments{};  // TODO: MAX_ARGS + 1 ?
    int currentArgument = 0;
    int argumentCount = 0;

    // Timing
    int gpuLine = 0;
    int gpuDot = 0;
    bool odd = false;
    int frames = 0;

    // TODO: Move Debug GUI stuff to class and befriend it
   public:
    // GPU Registers

    GP0_E1 gp0_e1;
    GP0_E2 gp0_e2;

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

    GP1_08 gp1_08;

    // GP1(0x09)
    bool textureDisableAllowed = false;

    std::array<uint16_t, VRAM_WIDTH * VRAM_HEIGHT> vram{};

   private:
    // Hardware rendering
    std::vector<Vertex> vertices;

    bool forceNtsc;
    bool softwareRendering;
    bool hardwareRendering;

    void reset();
    void cmdFillRectangle(uint8_t command);
    void cmdPolygon(PolygonArgs arg);
    void cmdLine(LineArgs arg);
    void cmdRectangle(RectangleArgs arg);
    void cmdCpuToVram1(uint8_t command);
    void cmdCpuToVram2(uint8_t command);
    void cmdVramToCpu(uint8_t command);
    void cmdVramToVram(uint8_t command);

    void drawTriangle(const primitive::Triangle& triangle);
    void drawLine(const primitive::Line& line);
    void drawRectangle(const primitive::Rect& rect);

    void writeGP0(uint32_t data);
    void writeGP1(uint32_t data);

    void reload();
    void maskedWrite(int x, int y, uint16_t value);

    uint32_t readVramWord();
    uint32_t getStat();

   public:
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

    // Debug && replay
    bool gpuLogEnabled = true;
    std::vector<LogEntry> gpuLogList;
    std::array<uint16_t, VRAM_WIDTH * VRAM_HEIGHT> prevVram{};

    void clear() { vertices.clear(); }
    void dumpVram();

    template <class Archive>
    void serialize(Archive& ar) {
        ar(startX, startY);
        ar(endX, endY);
        ar(currX, currY);
        ar(readMode, readData);

        ar(cmd, command);
        ar(arguments);
        ar(currentArgument, argumentCount);

        ar(gpuLine, gpuDot);
        ar(odd, frames);

        // GP0_xx
        ar(gp0_e1._reg);
        ar(gp0_e2._reg);
        ar(drawingArea);
        ar(drawingOffsetX, drawingOffsetY);
        ar(gp0_e6._reg);

        // GP1_xx
        ar(irqRequest);
        ar(displayDisable);
        ar(dmaDirection);
        ar(displayAreaStartX, displayAreaStartY);
        ar(displayRangeX1, displayRangeX2);
        ar(displayRangeY1, displayRangeY2);
        ar(gp1_08._reg);
        ar(textureDisableAllowed);

        ar(vram);
    }
};

}  // namespace gpu