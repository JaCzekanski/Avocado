#pragma once
#include <stdint.h>
#include "device/device.h"
#include <vector>

struct Matrix {
    int16_t v11 = 0;
    int16_t v12 = 0;
    int16_t v13 = 0;

    int16_t v21 = 0;
    int16_t v22 = 0;
    int16_t v23 = 0;

    int16_t v31 = 0;
    int16_t v32 = 0;
    int16_t v33 = 0;
};

template <typename T>
struct Vector {
    union {
        T x, r;
    };
    union {
        T y, g;
    };
    union {
        T z, b;
    };
};

struct Color {
    int32_t r = 0;
    int32_t g = 0;
    int32_t b = 0;
};

namespace mips {
namespace gte {

union Command {
    struct {
        uint32_t cmd : 6;
        uint32_t : 4;
        uint32_t lm : 1;
        uint32_t : 2;
        uint32_t mvmvaTranslationVector : 2;  // 0 - tr, 1 - bk, 2 - fc, 3 - none
        uint32_t mvmvaMultiplyVector : 2;     // 0 - v0, 1 - v1, 2 - v2, 3 - ir
        uint32_t mvmvaMultiplyMatrix : 2;     // 0 - rotation, 1 - light, 2 - color, 3 - reserved
        uint32_t sf : 1;
        uint32_t : 5;
        uint32_t : 7;  // cop2
    };
    uint32_t _reg;
    Command(uint32_t v) : _reg(v) {}
};

struct GTE {
    Vector<int16_t> v[3];
    device::Reg32 rgbc;
    uint16_t otz = 0;
    int16_t ir[4] = {0};
    Vector<int16_t> s[4];
    device::Reg32 rgb[3];
    uint32_t res1 = 0;  // prohibited
    int32_t mac[4] = {0};
    uint16_t irgb = 0;
    uint16_t orgb = 0;
    int32_t lzcs = 0;
    int32_t lzcr = 0;

    Matrix rt;
    Vector<int32_t> tr;
    Matrix l;
    Vector<int32_t> bk;
    Matrix lr;
    Vector<int32_t> fc;
    int32_t of[2] = {0};
    uint16_t h = 0;
    int16_t dqa = 0;
    int32_t dqb = 0;
    int16_t zsf3 = 0;
    int16_t zsf4 = 0;
    uint32_t flag = 0;

    uint32_t read(uint8_t n);
    void write(uint8_t n, uint32_t d);
    int32_t clip(int32_t value, int32_t min, int32_t max);

    void nclip();
    void ncds(bool sf, bool lm);
    int32_t divide(uint16_t h, uint16_t sz3);
    void rtps(int n, bool sf, bool lm);
    void rtpt(bool sf, bool lm);
    void avsz3();
    void avsz4();
    void mvmva(bool sf, bool lm, int mx, int vx, int tx);
    void gpf(bool sf, bool lm);
    void sqr(bool sf, bool lm);
    void op(bool sf, bool lm);

    struct GTE_ENTRY {
        enum class MODE { read, write, func } mode;
        uint32_t n;
        uint32_t data;
    };

    std::vector<GTE_ENTRY> log;
};
};
};
