#pragma once
#include "device.h"
#include <vector>

namespace device {
namespace gpu {

union PolygonArgs {
    struct {
        uint8_t : 1;
        uint8_t semiTransparency : 1;
        uint8_t isTextureMapped : 1;
        uint8_t isQuad : 1;
        uint8_t isShaded : 1;
        uint8_t : 3;
    };
    uint8_t _;

    PolygonArgs(uint8_t arg) : _(arg) {}

    int getArgumentCount() const { return (isQuad ? 4 : 3) * (isTextureMapped ? 2 : 1) * (isShaded ? 2 : 1) - (isShaded ? 1 : 0); }

    int getVertexCount() const { return isQuad ? 4 : 3; }
};

union LineArgs {
    struct {
        uint8_t : 1;
        uint8_t semiTransparency : 1;
        uint8_t : 1;
        uint8_t polyLine : 1;
        uint8_t isShaded : 1;
        uint8_t : 3;
    };
    uint8_t _;

    LineArgs(uint8_t arg) : _(arg) {}

    int getArgumentCount() const { return (polyLine ? 256 : (isShaded ? 2 : 1) * 2); }
};

union RectangleArgs {
    struct {
        uint8_t : 1;
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

enum class Command { Nop, FillRectangle, Polygon, Line, Rectangle, CopyCpuToVram, CopyVramToCpu, CopyVramToVram };

struct Vertex {
    int position[2];
    int color[3];
    int texcoord[2];
    int bitcount;
    int clut[2];     // clut position
    int texpage[2];  // texture page position
};

// Draw Mode setting
union GP0_E1 {
    enum class SemiTransparency : uint32_t {
        Bby2plusFby2 = 0,  // B/2+F/2
        BplusF = 1,        // B+F
        BminusF = 2,       // B-F
        BplusFby4 = 3      // B+F/4
    };
    enum class TexturePageColors : uint32_t { bit4 = 0, bit8 = 1, bit15 = 2 };
    enum class DrawingToDisplayArea : uint32_t { prohibited = 0, allowed = 1 };
    struct {
        uint32_t texturePageBaseX : 4;  // N * 64
        uint32_t texturePageBaseY : 1;  // N * 256
        SemiTransparency semiTransparency : 2;
        TexturePageColors texturePageColors : 2;
        Bit dither24to15 : 1;
        DrawingToDisplayArea drawingToDisplayArea : 1;
        Bit textureDisable : 1;          // 0=Normal, 1=Disable if GP1(09h).Bit0=1)
        Bit texturedRectangleXFlip : 1;  // (BIOS does set this bit on power-up...?)
        Bit texturedRectangleYFlip : 1;  // (BIOS does set it equal to GPUSTAT.13...?)

        uint32_t : 10;
        uint8_t command;  // 0xe1
    };
    struct {
        uint32_t _reg : 24;
        uint32_t _command : 8;
    };

    GP0_E1() : _reg(0) {}
};

// Display mode
union GP1_08 {
    enum class HorizontalResolution : uint8_t { r256 = 0, r320 = 1, r512 = 2, r640 = 3 };
    enum class VerticalResolution : uint8_t {
        r240 = 0,
        r480 = 1,  // if VerticalInterlace
    };
    enum class VideoMode : uint8_t {
        ntsc = 0,  // 60hz
        pal = 1,   // 50hz
    };
    enum class ColorDepth : uint8_t { bit15 = 0, bit24 = 1 };
    enum class HorizontalResolution2 : uint8_t {
        normal = 0,
        r386 = 1,
    };
    enum class TexturePageColors : uint8_t { bit4 = 0, bit8 = 1, bit15 = 2 };
    enum class DrawingToDisplayArea : uint8_t { prohibited = 0, allowed = 1 };
    enum class ReverseFlag : uint8_t { normal = 0, distorted = 1 };
    struct {
        HorizontalResolution horizontalResolution1 : 2;
        VerticalResolution verticalResolution : 1;
        VideoMode videoMode : 1;
        ColorDepth colorDepth : 1;
        uint8_t interlace : 1;
        HorizontalResolution2 horizontalResolution2 : 1;  // (0=256/320/512/640, 1=368)
        ReverseFlag reverseFlag : 1;

        uint32_t : 16;
        uint8_t command;  // 0x08
    };
    struct {
        uint32_t _reg : 24;
        uint32_t _command : 8;
    };

    GP1_08() : _reg(0) {}

    int getHorizontalResoulution() {
        if (horizontalResolution2 == HorizontalResolution2::r386) return 368;
        switch (horizontalResolution1) {
            case HorizontalResolution::r256:
                return 256;
            case HorizontalResolution::r320:
                return 320;
            case HorizontalResolution::r512:
                return 512;
            case HorizontalResolution::r640:
                return 640;
        }
    }

    int getVerticalResoulution() {
        if (verticalResolution == VerticalResolution::r240) return 240;
        return 480;
    }
};

class GPU {
    /* 0 - nothing
       1 - GP0(0xc0) - VRAM to CPU transfer
       2 - GP1(0x10) - Get GPU Info
    */
    int startX = 0;
    int startY = 0;
    int endX = 0;
    int endY = 0;
    int currX = 0;
    int currY = 0;
    int gpuReadMode = 0;
    uint32_t GPUREAD = 0;
    uint32_t GPUSTAT = 0;

    const float WIDTH = 640.f;
    const float HEIGHT = 480.f;

    // GP0(0xc0)
    bool readyVramToCpu = false;

    GP0_E1 gp0_e1;

    // GP0(0xe2)
    int textureWindowMaskX = 0;
    int textureWindowMaskY = 0;
    int textureWindowOffsetX = 0;
    int textureWindowOffsetY = 0;

   public:
    // GP0(0xe3)
    int drawingAreaX1 = 0;
    int drawingAreaY1 = 0;

    // GP0(0xe4)
    int drawingAreaX2 = 0;
    int drawingAreaY2 = 0;

    // GP0(0xe5)
    int drawingOffsetX = 0;
    int drawingOffsetY = 0;

   private:
    // GP0(0xe6)
    int setMaskWhileDrawing = 0;
    int checkMaskBeforeDraw = 0;

    // GP1(0x02)
    Bit irqAcknowledge;

    // GP1(0x03)
    Bit displayDisable;

    // GP(0x04)
    int dmaDirection = 0;

    // GP1(0x05)
   public:
    int displayAreaStartX = 0;
    int displayAreaStartY = 0;

   private:
    // GP1(0x06)
    int displayRangeX1 = 0;
    int displayRangeX2 = 0;

    // GP1(0x07)
    int displayRangeY1 = 0;
    int displayRangeY2 = 0;

   public:
    GP1_08 gp1_08;

   private:
    // GP1(0x09)
    bool textureDisableAllowed = false;

    uint32_t to15bit(uint32_t color);
    uint32_t to24bit(uint16_t color);

    void cmdFillRectangle(const uint8_t command, uint32_t argument, uint32_t arguments[32]);
    void cmdPolygon(const PolygonArgs arg, uint32_t argument, uint32_t arguments[]);
    void cmdLine(const LineArgs arg, uint32_t argument, uint32_t arguments[]);
    void cmdRectangle(const RectangleArgs command, uint32_t argument, uint32_t arguments[32]);
    void cmdVramToCpu(uint8_t command, uint32_t argument, uint32_t arguments[32]);
    void cmdVramToVram(uint8_t command, uint32_t argument, uint32_t arguments[32]);

    void drawPolygon(int x[4], int y[4], int c[4], int t[4] = nullptr, bool isFourVertex = false, bool textured = false);

    void writeGP0(uint32_t data);
    void writeGP1(uint32_t data);

   public:
    bool odd = false;
    int frames = 0;
    void step();
    uint32_t read(uint32_t address);
    void write(uint32_t address, uint32_t data);

    std::vector<Vertex>& render();

    std::vector<Vertex> renderList;
    uint16_t VRAM[512][1024];
};
}
}
