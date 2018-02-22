#pragma once
#include <glm/vec2.hpp>
#include <vector>
#include "psx_color.h"
#include "registers.h"

const int MAX_ARGS = 32;

extern const char* CommandStr[];

#define VRAM ((uint16_t(*)[GPU::VRAM_WIDTH])vram.data())

union PolygonArgs {
    struct {
        uint8_t isRawTexture : 1;
        uint8_t semiTransparency : 1;
        uint8_t isTextureMapped : 1;
        uint8_t isQuad : 1;
        uint8_t gouroudShading : 1;
        uint8_t : 3;
    };
    uint8_t _;

    PolygonArgs(uint8_t arg) : _(arg) {}

    int getArgumentCount() const {
        int size = isQuad ? 4 : 3;
        if (isTextureMapped) size *= 2;
        if (gouroudShading) size += (isQuad ? 4 : 3) - 1;

        return size;
    }

    int getVertexCount() const { return isQuad ? 4 : 3; }
};

union LineArgs {
    struct {
        uint8_t : 1;
        uint8_t semiTransparency : 1;
        uint8_t : 1;
        uint8_t polyLine : 1;
        uint8_t gouroudShading : 1;
        uint8_t : 3;
    };
    uint8_t _;

    LineArgs(uint8_t arg) : _(arg) {}

    int getArgumentCount() const { return (polyLine ? MAX_ARGS - 1 : (gouroudShading ? 2 : 1) * 2); }
};

union RectangleArgs {
    struct {
        uint8_t isRawTexture : 1;
        uint8_t semiTransparency : 1;
        uint8_t isTextureMapped : 1;
        uint8_t size : 2;
        uint8_t : 3;
    };
    uint8_t _;

    RectangleArgs(uint8_t arg) : _(arg) {}

    int getArgumentCount() const { return (size == 0 ? 2 : 1) + (isTextureMapped ? 1 : 0); }

    int getSize() const {
        if (size == 1) return 1;
        if (size == 2) return 8;
        if (size == 3) return 16;
        return 0;
    }
};

enum class Command : int {
    None,
    FillRectangle,
    Polygon,
    Line,
    Rectangle,
    CopyCpuToVram1,
    CopyCpuToVram2,
    CopyVramToCpu,
    CopyVramToVram,
    Extra
};

struct Vertex {
    enum Flags { SemiTransparency = 1 << 0, RawTexture = 1 << 1, Dithering = 1 << 2 };
    int position[2];
    int color[3];
    int texcoord[2];
    int bitcount;
    int clut[2];     // clut position
    int texpage[2];  // texture page position
    int flags;
    /**
     * 0b76543210
     *          ^
     *          Transparency enabled (yes for tris, no for rect)
     */
};

struct TextureInfo {
    // t[0] ClutYyXx
    // t[1] PageYyXx
    // t[2] 0000YyXx
    // t[3] 0000YyXx

    uint32_t palette;
    uint32_t texpage;
    glm::ivec2 uv[4];

    int getClutX() const { return ((palette & 0x003f0000) >> 16) * 16; }

    int getClutY() const { return ((palette & 0x7fc00000) >> 22); }

    int getBaseX() const {
        return ((texpage & 0x0f0000) >> 16) * 64;  // N * 64
    }

    int getBaseY() const {
        return ((texpage & 0x100000) >> 20) * 256;  // N * 256
    }

    int getBitcount() const {
        int depth = (texpage & 0x1800000) >> 23;
        switch (depth) {
            case 0:
                return 4;
            case 1:
                return 8;
            case 2:
                return 16;
            case 3:
                return 16;
            default:
                return 0;
        }
    }

    bool isTransparent() const { return (texpage & 0x600000) >> 21; }
};

struct GPU {
    /* 0 - nothing
       1 - GP0(0xc0) - VRAM to CPU transfer
       2 - GP1(0x10) - Get GPU Info
    */
    static const int VRAM_WIDTH = 1024;
    static const int VRAM_HEIGHT = 512;

    int resolutionMultiplier = 1;

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
    uint32_t arguments[33];
    int currentArgument = 0;
    int argumentCount = 0;

    GP0_E1 gp0_e1;
    GP0_E2 gp0_e2;

    // GP0(0xe3)
    int16_t drawingAreaLeft = 0;
    int16_t drawingAreaTop = 0;

    // GP0(0xe4)
    int16_t drawingAreaRight = 0;
    int16_t drawingAreaBottom = 0;

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

    void reset();
    void cmdFillRectangle(uint8_t command, uint32_t arguments[]);
    void cmdPolygon(PolygonArgs arg, uint32_t arguments[]);
    void cmdLine(LineArgs arg, uint32_t arguments[]);
    void cmdRectangle(RectangleArgs arg, uint32_t arguments[]);
    void cmdCpuToVram1(uint8_t command, uint32_t arguments[]);
    void cmdCpuToVram2(uint8_t command, uint32_t arguments[]);
    void cmdVramToCpu(uint8_t command, uint32_t arguments[]);
    void cmdVramToVram(uint8_t command, uint32_t arguments[]);

    void drawPolygon(int16_t x[4], int16_t y[4], RGB c[4], TextureInfo t, bool isFourVertex = false, bool textured = false, int flags = 0);

    void writeGP0(uint32_t data);
    void writeGP1(uint32_t data);

    int minDrawingX(int x) const;
    int minDrawingY(int y) const;
    int maxDrawingX(int x) const;
    int maxDrawingY(int y) const;
    bool insideDrawingArea(int x, int y) const;

    bool odd = false;
    int frames = 0;

    GPU() {
        vram.resize(VRAM_WIDTH * VRAM_HEIGHT * resolutionMultiplier);
        prevVram.resize(VRAM_WIDTH * VRAM_HEIGHT * resolutionMultiplier);
    }
    void step();
    uint32_t read(uint32_t address);
    void write(uint32_t address, uint32_t data);

    bool emulateGpuCycles(int cycles);

    std::vector<uint16_t> vram;
    std::vector<uint16_t> prevVram;

    struct GPU_LOG_ENTRY {
        uint8_t command;
        Command cmd;
        std::vector<uint32_t> args;
    };

    bool gpuLogEnabled = true;

    std::vector<GPU_LOG_ENTRY> gpuLogList;
};