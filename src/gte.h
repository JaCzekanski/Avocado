#pragma once
#include <stdint.h>
#include "device/device.h"

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

struct Vector {
    int16_t x = 0;
    int16_t y = 0;
    int16_t z = 0;
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
    Vector v[3];
    device::Reg32 rgbc;
    uint16_t otz = 0;
    int16_t ir[4] = {0};
    Vector s[4] = {0};
    device::Reg32 rgb[3];
    uint32_t res1 = 0;  // prohibited
    int32_t mac[4] = {0};
    uint16_t irgb = 0;
    uint16_t orgb = 0;
    int32_t lzcs = 0;
    int32_t lzcr = 0;

    Matrix rt;
    Vector tr;
    Matrix l;
    Color bk = {0};
    Matrix lr;
    Color fc = {0};
    uint32_t of[2] = {0};
    uint16_t h = 0;
    int16_t dqa = 0;
    uint32_t dqb = 0;
    int16_t zsf3 = 0;
    int16_t zsf4 = 0;
    uint32_t flag = 0;

    uint32_t read(uint8_t n) {
        switch (n) {
            case 0:
                return (v[0].y << 16) | v[0].x;
            case 1:
                return v[0].z;
            case 2:
                return (v[1].y << 16) | v[1].x;
            case 3:
                return v[1].z;
            case 4:
                return (v[2].y << 16) | v[2].x;
            case 5:
                return v[2].z;
            case 6:
                return rgbc._reg;
            case 7:
                return otz;

            case 8:
                return ir[0];
            case 9:
                return ir[1];
            case 10:
                return ir[2];
            case 11:
                return ir[3];
            case 12:
                return (s[0].y << 16) | s[0].x;
            case 13:
                return (s[1].y << 16) | s[1].x;
            case 14:
                return (s[2].y << 16) | s[2].x;
            case 15:
                return (s[3].y << 16) | s[3].x;

            case 16:
                return s[0].z;
            case 17:
                return s[1].z;
            case 18:
                return s[2].z;
            case 19:
                return s[3].z;
            case 20:
                return rgb[0]._reg;
            case 21:
                return rgb[1]._reg;
            case 22:
                return rgb[2]._reg;
            case 23:
                return res1;

            case 24:
                return mac[0];
            case 25:
                return mac[1];
            case 26:
                return mac[2];
            case 27:
                return mac[3];
            case 28:
                return irgb;
            case 29:
                return orgb;
            case 30:
                return lzcs;
            case 31:
                return lzcr;

            case 32:
                return (rt.v12 << 16) | rt.v11;
            case 33:
                return (rt.v21 << 16) | rt.v13;
            case 34:
                return (rt.v23 << 16) | rt.v22;
            case 35:
                return (rt.v32 << 16) | rt.v31;
            case 36:
                return (int32_t)rt.v33;
            case 37:
                return tr.x;
            case 38:
                return tr.y;
            case 39:
                return tr.z;

            case 40:
                return (l.v12 << 16) | l.v11;
            case 41:
                return (l.v21 << 16) | l.v13;
            case 42:
                return (l.v23 << 16) | l.v22;
            case 43:
                return (l.v32 << 16) | l.v31;
            case 44:
                return (int32_t)l.v33;
            case 45:
                return bk.r;
            case 46:
                return bk.g;
            case 47:
                return bk.b;

            case 48:
                return (lr.v12 << 16) | lr.v11;
            case 49:
                return (lr.v21 << 16) | lr.v13;
            case 50:
                return (lr.v23 << 16) | lr.v22;
            case 51:
                return (lr.v32 << 16) | lr.v31;
            case 52:
                return (int32_t)lr.v33;
            case 53:
                return fc.r;
            case 54:
                return fc.g;
            case 55:
                return fc.b;

            case 56:
                return of[0];
            case 57:
                return of[1];
            case 58:
                return h;
            case 59:
                return dqa;
            case 60:
                return dqb;
            case 61:
                return zsf3;
            case 62:
                return zsf4;
            case 63:
                return flag;
            default:
                return 0;
        }
    }

    void write(uint8_t n, uint32_t d) {
        switch (n) {
            case 0:
                v[0].y = d >> 16;
                v[0].x = d;
                break;
            case 1:
                v[0].z = d;
                break;
            case 2:
                v[1].y = d >> 16;
                v[1].x = d;
                break;
            case 3:
                v[1].z = d;
                break;
            case 4:
                v[2].y = d >> 16;
                v[2].x = d;
                break;
            case 5:
                v[2].z = d;
                break;
            case 6:
                rgbc._reg = d;
                break;
            case 7:
                otz = d;
                break;

            case 8:
                ir[0] = d;
                break;
            case 9:
                ir[1] = d;
                break;
            case 10:
                ir[2] = d;
                break;
            case 11:
                ir[3] = d;
                break;
            case 12:
                s[0].y = d >> 16;
                s[0].x = d;
                break;
            case 13:
                s[1].y = d >> 16;
                s[1].x = d;
                break;
            case 14:
                s[2].y = d >> 16;
                s[2].x = d;
                break;
            case 15:
                s[3].y = d >> 16;
                s[3].x = d;
                break;

            case 16:
                s[0].z = d;
                break;
            case 17:
                s[1].z = d;
                break;
            case 18:
                s[2].z = d;
                break;
            case 19:
                s[3].z = d;
                break;
            case 20:
                rgb[0]._reg = d;
                break;
            case 21:
                rgb[1]._reg = d;
                break;
            case 22:
                rgb[2]._reg = d;
                break;
            case 23:
                res1 = d;
                break;

            case 24:
                mac[0] = d;
                break;
            case 25:
                mac[1] = d;
                break;
            case 26:
                mac[2] = d;
                break;
            case 27:
                mac[3] = d;
                break;
            case 28:
                irgb = d;
                break;
            case 29:
                orgb = d;
                break;
            case 30:
                lzcs = d;
                break;
            case 31:
                lzcr = d;
                break;

            case 32:
                rt.v12 = d >> 16;
                rt.v11 = d;
                break;
            case 33:
                rt.v21 = d >> 16;
                rt.v13 = d;
                break;
            case 34:
                rt.v23 = d >> 16;
                rt.v22 = d;
                break;
            case 35:
                rt.v32 = d >> 16;
                rt.v31 = d;
                break;
            case 36:
                rt.v33 = d;
                break;
            case 37:
                tr.x = d;
                break;
            case 38:
                tr.y = d;
                break;
            case 39:
                tr.z = d;
                break;

            case 40:
                l.v12 = d >> 16;
                l.v11 = d;
                break;
            case 41:
                l.v21 = d >> 16;
                l.v13 = d;
                break;
            case 42:
                l.v23 = d >> 16;
                l.v22 = d;
                break;
            case 43:
                l.v32 = d >> 16;
                l.v31 = d;
                break;
            case 44:
                l.v33 = d;
                break;
            case 45:
                bk.r = d;
                break;
            case 46:
                bk.g = d;
                break;
            case 47:
                bk.b = d;
                break;

            case 48:
                lr.v12 = d >> 16;
                lr.v11 = d;
                break;
            case 49:
                lr.v21 = d >> 16;
                lr.v13 = d;
                break;
            case 50:
                lr.v23 = d >> 16;
                lr.v22 = d;
                break;
            case 51:
                lr.v32 = d >> 16;
                lr.v31 = d;
                break;
            case 52:
                lr.v33 = d;
                break;
            case 53:
                fc.r = d;
                break;
            case 54:
                fc.g = d;
                break;
            case 55:
                fc.b = d;
                break;

            case 56:
                of[0] = d;
                break;
            case 57:
                of[1] = d;
                break;
            case 58:
                h = d;
                break;
            case 59:
                dqa = d;
                break;
            case 60:
                dqb = d;
                break;
            case 61:
                zsf3 = d;
                break;
            case 62:
                zsf4 = d;
                break;
            case 63:
                flag = d;
                break;
            default:
                return;
        }
    }

#define A1(x) (x)
#define A2(x) (x)
#define A3(x) (x)

#define Lm_B1(x) (x)
#define Lm_B2(x) (x)
#define Lm_B3(x) (x)

#define Lm_C1(x) (x)
#define Lm_C2(x) (x)
#define Lm_C3(x) (x)

#define Lm_D(x, sf) ((x) >> ((1 - sf) * 12))

#define Lm_G1(x) (x)
#define Lm_G2(x) (x)

#define F(x) (x)

#define Lm_H(x) ((x) / 0x1000)

#define Lm_E(x) (x)

#define TRX ((int32_t)tr.x)
#define TRY ((int32_t)tr.y)
#define TRZ ((int32_t)tr.z)

#define R11 ((int32_t)rt.v11)
#define R12 ((int32_t)rt.v12)
#define R13 ((int32_t)rt.v13)

#define R21 ((int32_t)rt.v21)
#define R22 ((int32_t)rt.v22)
#define R23 ((int32_t)rt.v23)

#define R31 ((int32_t)rt.v31)
#define R32 ((int32_t)rt.v32)
#define R33 ((int32_t)rt.v33)

    void nclip() {
        mac[0] = F((int32_t)s[0].x * s[1].y + (int32_t)s[1].x * s[2].y + (int32_t)s[2].x * s[0].y - (int32_t)s[0].x * s[2].y
                   - (int32_t)s[1].x * s[0].y - (int32_t)s[2].x * s[1].y);
    }

    void ncds(bool sf) {
        // TODO
        mac[1] = A1((uint32_t)l.v11 * v[0].x + (uint32_t)l.v12 * v[0].y + (uint32_t)l.v13 * v[0].z) >> (sf * 12);
        mac[2] = A2((uint32_t)l.v21 * v[0].x + (uint32_t)l.v22 * v[0].y + (uint32_t)l.v23 * v[0].z) >> (sf * 12);
        mac[3] = A3((uint32_t)l.v31 * v[0].x + (uint32_t)l.v32 * v[0].y + (uint32_t)l.v33 * v[0].z) >> (sf * 12);

        ir[1] = Lm_B1(mac[1]);
        ir[2] = Lm_B2(mac[2]);
        ir[3] = Lm_B3(mac[3]);

        mac[1] = A1((bk.r << 12) + (uint32_t)lr.v11 * ir[1] + (uint32_t)lr.v12 * ir[2] + (uint32_t)lr.v13 * ir[3]) >> (sf * 12);
        mac[2] = A2((bk.g << 12) + (uint32_t)lr.v21 * ir[1] + (uint32_t)lr.v22 * ir[2] + (uint32_t)lr.v23 * ir[3]) >> (sf * 12);
        mac[3] = A3((bk.b << 12) + (uint32_t)lr.v31 * ir[1] + (uint32_t)lr.v32 * ir[2] + (uint32_t)lr.v33 * ir[3]) >> (sf * 12);

        ir[1] = Lm_B1(mac[1]);
        ir[2] = Lm_B2(mac[2]);
        ir[3] = Lm_B3(mac[3]);

#define R ((uint32_t)(rgbc.read(0) << 4))
#define G ((uint32_t)(rgbc.read(1) << 4))
#define B ((uint32_t)(rgbc.read(2) << 4))
#define CODE ((uint32_t)(rgbc.read(3) << 4))

        mac[1] = A1(R * ir[1] + (uint32_t)ir[0] * Lm_B1((fc.r << 12) - R * ir[1])) >> (sf * 12);
        mac[2] = A2(G * ir[2] + (uint32_t)ir[0] * Lm_B2((fc.g << 12) - G * ir[2])) >> (sf * 12);
        mac[3] = A3(B * ir[3] + (uint32_t)ir[0] * Lm_B3((fc.b << 12) - B * ir[3])) >> (sf * 12);

#undef R G B CODE

        ir[1] = Lm_B1(mac[1]);
        ir[2] = Lm_B2(mac[2]);
        ir[3] = Lm_B3(mac[3]);

        rgb[0] = rgb[1];
        rgb[1] = rgb[2];

        rgb[2].write(0, Lm_C1(mac[1] >> 4));  // R
        rgb[2].write(1, Lm_C2(mac[2] >> 4));  // G
        rgb[2].write(2, Lm_C3(mac[3] >> 4));  // B
        rgb[2].write(3, CODE);                // CODE
    }

    int32_t divide(int32_t a, int32_t b) {
        if (b == 0) {
            // TODO Flag.Bit17 = 1
            // TODO Flag.Bit31 = 1
            return 0x1ffff;
        }
        int32_t r = Lm_E(((a * 0x20000 / b) + 1) / 2);
        if (r > 0x1ffff) {
            // TODO Flag.Bit17 = 1
            // TODO Flag.Bit31 = 1
            return 0x1ffff;
        }
        return r;
    }

    void rtps(int n, bool sf) {
        mac[1] = A1((TRX << 12) + R11 * v[n].x + R12 * v[n].y + R13 * v[n].z) >> (sf * 12);
        mac[2] = A2((TRY << 12) + R21 * v[n].x + R22 * v[n].y + R23 * v[n].z) >> (sf * 12);
        mac[3] = A3((TRZ << 12) + R31 * v[n].x + R32 * v[n].y + R33 * v[n].z) >> (sf * 12);
        ir[1] = Lm_B1(mac[1]);
        ir[2] = Lm_B2(mac[2]);
        ir[3] = Lm_B3(mac[3]);

        s[0].z = s[1].z;
        s[1].z = s[2].z;
        s[2].z = s[3].z;

        s[3].z = Lm_D(mac[3], sf);

        s[0].x = s[1].x;
        s[0].y = s[1].y;
        s[1].x = s[2].x;
        s[1].y = s[2].y;

        int32_t h_s3z = divide(h, s[3].z);

        s[2].x = Lm_G1(F(of[0] + (int32_t)ir[1] * h_s3z) / 0x10000);
        s[2].y = Lm_G2(F(of[1] + (int32_t)ir[2] * h_s3z) / 0x10000);

        mac[0] = F(dqb + (int32_t)dqa * h_s3z);
        ir[0] = Lm_H(mac[0]);
    }

    void rtpt(bool sf) {
        rtps(0, sf);
        rtps(1, sf);
        rtps(2, sf);
    }

    void avsz3() {
        mac[0] = F((int32_t)zsf3 * ((int32_t)s[1].z + (int32_t)s[2].z + (int32_t)s[3].z));
        otz = Lm_D(mac[0], 1);
    }

    void avsz4() {
        mac[0] = F((int32_t)zsf4 * ((int32_t)s[0].z + (int32_t)s[1].z + (int32_t)s[2].z + (int32_t)s[3].z));
        otz = Lm_D(mac[0], 1);
    }
};
};
};
