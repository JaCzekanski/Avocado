#pragma once
#include "device/device.h"

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

// Texture Window setting
union GP0_E2 {
    struct {
        uint32_t textureWindowMaskX : 5;
        uint32_t textureWindowMaskY : 5;
        uint32_t textureWindowOffsetX : 5;
        uint32_t textureWindowOffsetY : 5;

        uint32_t : 4;
        uint8_t command;  // 0xe1
    };
    struct {
        uint32_t _reg : 24;
        uint32_t _command : 8;
    };

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
    struct {
        uint32_t _reg : 24;
        uint32_t _command : 8;
    };

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