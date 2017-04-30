#pragma once
#include <stdint.h>
#include "device/device.h"

struct Matrix {
    int16_t v11;
    int16_t v12;
    int16_t v13;

    int16_t v21;
    int16_t v22;
    int16_t v23;

    int16_t v31;
    int16_t v32;
    int16_t v33;
};

struct Vector {
    int16_t x;
    int16_t y;
    int16_t z;
};

namespace mips {
namespace gte {

struct GTE {
    Vector v[3];
    device::Reg32 rgbc;
    uint16_t otz;
    int16_t ir[4];
    Vector s[4];
    device::Reg32 rgb[3];
    uint32_t res1;  // prohibited
    uint32_t mac[4];
    uint16_t irgb;
    uint16_t orgb;
    int32_t lzcs;
    int32_t lzcr;

    Matrix rt;
    Vector tr;
    Matrix l;
    uint32_t bk[3];
    Matrix lr;
    uint32_t fc[3];
    uint32_t of[2];
    uint16_t h;
    int16_t dqa;
    uint32_t dqb;
    int16_t zsf3;
    int16_t zsf4;
    uint32_t flag;

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
                return rt.v33;
            case 37:
                tr.x;
            case 38:
                tr.y;
            case 39:
                tr.z;

            case 40:
                return (l.v12 << 16) | l.v11;
            case 41:
                return (l.v21 << 16) | l.v13;
            case 42:
                return (l.v23 << 16) | l.v22;
            case 43:
                return (l.v32 << 16) | l.v31;
            case 44:
                return l.v33;
            case 45:
                return bk[0];
            case 46:
                return bk[1];
            case 47:
                return bk[2];

            case 48:
                return (lr.v12 << 16) | lr.v11;
            case 49:
                return (lr.v21 << 16) | lr.v13;
            case 50:
                return (lr.v23 << 16) | lr.v22;
            case 51:
                return (lr.v32 << 16) | lr.v31;
            case 52:
                return lr.v33;
            case 53:
                return fc[0];
            case 54:
                return fc[1];
            case 55:
                return fc[2];

            case 56:
                return of[0];
            case 57:
                return of[1];
            case 58:
                return h;
            case 59:
                return dqa;
            case 60:
                return dqa;
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
                bk[0] = d;
                break;
            case 46:
                bk[1] = d;
                break;
            case 47:
                bk[2] = d;
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
                fc[0] = d;
                break;
            case 54:
                fc[1] = d;
                break;
            case 55:
                fc[2] = d;
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
                dqa = d;
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

    void nclip() { mac[0] = s[0].x * s[1].y + s[1].x * s[2].y + s[2].x * s[0].y - s[0].x * s[2].y - s[1].x * s[0].y - s[2].x * s[1].y; }

    void ncds() {}

    void rtps(int n, bool sf) {
        ir[1] = mac[1]
            = ((int32_t)tr.x * 0x1000 + (int32_t)rt.v11 * v[n].x + (int32_t)rt.v12 * v[n].y + (int32_t)rt.v13 * v[n].z) >> (sf * 12);
        ir[2] = mac[2]
            = ((int32_t)tr.y * 0x1000 + (int32_t)rt.v21 * v[n].x + (int32_t)rt.v22 * v[n].y + (int32_t)rt.v23 * v[n].z) >> (sf * 12);
        ir[3] = mac[3]
            = ((int32_t)tr.z * 0x1000 + (int32_t)rt.v31 * v[n].x + (int32_t)rt.v32 * v[n].y + (int32_t)rt.v33 * v[n].z) >> (sf * 12);
        s[3].z = mac[3] >> ((1 - sf) * 12);
        if (s[3].z == 0) s[3].z = 1;  // lol xd

        mac[0] = (((h * 0x20000 / s[3].z) + 1) / 2) * ir[1] + of[0];
        s[2].x = mac[0] / 0x10000;
        mac[0] = (((h * 0x20000 / s[3].z) + 1) / 2) * ir[2] + of[1];
        s[2].y = mac[0] / 0x10000;
        mac[0] = (((h * 0x20000 / s[3].z) + 1) / 2) * dqa + dqb;
        ir[0] = mac[0] / 0x1000;
    }

    void rtpt(bool sf) {
        rtps(0, sf);
        rtps(1, sf);
        rtps(2, sf);
        // mac 0
    }
};
};
};
