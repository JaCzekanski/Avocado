#pragma once
#include "device/device.h"
#include "semi_transparency.h"

namespace gpu {

// FIFO size
const int MAX_ARGS = 32;

enum class ReadMode { Register, Vram };

// Draw Mode setting
union GP0_E1 {
    enum class TexturePageColors : uint32_t { bit4 = 0, bit8 = 1, bit15 = 2, reserved = 3 };
    enum class DrawingToDisplayArea : uint32_t { prohibited = 0, allowed = 1 };
    struct {
        uint32_t texturePageBaseX : 4;  // N * 64
        uint32_t texturePageBaseY : 1;  // N * 256
        SemiTransparency semiTransparency : 2;
        TexturePageColors texturePageColors : 2;
        uint32_t dither24to15 : 1;
        DrawingToDisplayArea drawingToDisplayArea : 1;
        uint32_t textureDisable : 1;          // 0=Normal, 1=Disable if GP1(09h).Bit0=1)
        uint32_t texturedRectangleXFlip : 1;  // (BIOS does set this bit on power-up...?)
        uint32_t texturedRectangleYFlip : 1;  // (BIOS does set it equal to GPUSTAT.13...?)

        uint32_t : 10;
        uint8_t command;  // 0xe1
    };
    uint32_t _reg;

    GP0_E1() : _reg(0) {}
};

// Texture Window setting
union GP0_E2 {
    struct {
        uint32_t maskX : 5;
        uint32_t maskY : 5;
        uint32_t offsetX : 5;
        uint32_t offsetY : 5;

        uint32_t : 4;
        uint8_t command;  // 0xe1
    };
    uint32_t _reg;

    GP0_E2() : _reg(0) {}
};

// Texture Window setting
union GP0_E6 {
    struct {
        uint32_t setMaskWhileDrawing : 1;
        uint32_t checkMaskBeforeDraw : 1;

        uint32_t : 22;
        uint8_t command;  // 0xe6
    };
    uint32_t _reg;

    GP0_E6() : _reg(0) {}
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
    uint32_t _reg;

    GP1_08() : _reg(0) {}

    int getHorizontalResoulution() {
        if (horizontalResolution2 == HorizontalResolution2::r386) return 368;
        if (horizontalResolution1 == HorizontalResolution::r256) return 256;
        if (horizontalResolution1 == HorizontalResolution::r320) return 320;
        if (horizontalResolution1 == HorizontalResolution::r512) return 512;
        if (horizontalResolution1 == HorizontalResolution::r640) return 640;
        return 640;
    }

    int getVerticalResoulution() {
        if (verticalResolution == VerticalResolution::r240) return 240;
        return 480;
    }
};

template <typename T>
struct Rect {
    T left = 0;
    T top = 0;
    T right = 0;
    T bottom = 0;

    template <class Archive>
    void serialize(Archive& ar) {
        ar(left, top, right, bottom);
    }
};

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

    int getArgumentCount() const {
        if (gouroudShading) {
            return 3;
        } else {
            return 2;
        }
    }
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
    enum Flags { SemiTransparency = 1 << 0, RawTexture = 1 << 1, Dithering = 1 << 2, GouroudShading = 1 << 3 };

    float position[2];
    int color[3];
    int texcoord[2];
    int bitcount;
    int clut[2];     // clut position
    int texpage[2];  // texture page position
    int flags;
    GP0_E2 textureWindow;
    GP0_E6 maskSettings;

    /**
     * 0b76543210
     *    ^^ ^^^^
     *    || |Flags
     *    |/
     *    |Transparency mode
     */
};

struct TextureInfo {
    // t[0] ClutYyXx
    // t[1] PageYyXx
    // t[2] 0000YyXx
    // t[3] 0000YyXx

    uint32_t palette;
    uint32_t texpage;
    ivec2 uv[4];

    int getClutX() const { return ((palette & 0x003f0000) >> 16) * 16; }
    int getClutY() const { return ((palette & 0x7fc00000) >> 22); }
    int getBaseX() const { return ((texpage & 0x0f0000) >> 16) * 64; }   // N * 64
    int getBaseY() const { return ((texpage & 0x100000) >> 20) * 256; }  // N * 256
    int getBitcount() const {
        using Bits = gpu::GP0_E1::TexturePageColors;
        Bits depth = (Bits)((texpage & 0x1800000) >> 23);
        switch (depth) {
            case Bits::bit4: return 4;
            case Bits::bit8: return 8;
            case Bits::bit15: return 16;
            case Bits::reserved: return 16;
            default: return 0;
        }
    }

    SemiTransparency semiTransparencyBlending() const { return (SemiTransparency)((texpage & 0x600000) >> 21); }
};

// Debug/rewind
struct LogEntry {
    uint8_t command;
    Command cmd;
    std::vector<uint32_t> args;
};

}  // namespace gpu