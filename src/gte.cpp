#include "gte.h"
#include <cmath>
#include <cstdio>
#include <cassert>

namespace mips {
namespace gte {
uint32_t GTE::read(uint8_t n) {
    switch (n) {
        // Data
        case 0:
            return (v[0].y << 16) | v[0].x;
        case 1:
            return (int32_t)v[0].z;
        case 2:
            return (v[1].y << 16) | v[1].x;
        case 3:
            return (int32_t)v[1].z;
        case 4:
            return (v[2].y << 16) | v[2].x;
        case 5:
            return (int32_t)v[2].z;
        case 6:
            return rgbc._reg;
        case 7:
            return otz;

        case 8:
            return (int32_t)ir[0];
        case 9:
            return (int32_t)ir[1];
        case 10:
            return (int32_t)ir[2];
        case 11:
            return (int32_t)ir[3];
        case 12:
            return ((uint16_t)s[0].y << 16) | (uint16_t)s[0].x;
        case 13:
            return ((uint16_t)s[1].y << 16) | (uint16_t)s[1].x;
        case 14:
        case 15:
            return ((uint16_t)s[2].y << 16) | (uint16_t)s[2].x;

        case 16:
            return (uint16_t)s[0].z;
        case 17:
            return (uint16_t)s[1].z;
        case 18:
            return (uint16_t)s[2].z;
        case 19:
            return (uint16_t)s[3].z;
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
        case 29:
            irgb = clip(ir[1] / 0x80, 0x1f, 0x00);
            irgb |= clip(ir[2] / 0x80, 0x1f, 0x00) << 5;
            irgb |= clip(ir[3] / 0x80, 0x1f, 0x00) << 10;
            return irgb;
        case 30:
            return lzcs;
        case 31:
            return lzcr;

        // Control
        case 32:
            return ((uint16_t)rt.v12 << 16) | (uint16_t)rt.v11;
        case 33:
            return ((uint16_t)rt.v21 << 16) | (uint16_t)rt.v13;
        case 34:
            return ((uint16_t)rt.v23 << 16) | (uint16_t)rt.v22;
        case 35:
            return ((uint16_t)rt.v32 << 16) | (uint16_t)rt.v31;
        case 36:
            return (int32_t)rt.v33;
        case 37:
            return tr.x;
        case 38:
            return tr.y;
        case 39:
            return tr.z;

        case 40:
            return ((uint16_t)l.v12 << 16) | (uint16_t)l.v11;
        case 41:
            return ((uint16_t)l.v21 << 16) | (uint16_t)l.v13;
        case 42:
            return ((uint16_t)l.v23 << 16) | (uint16_t)l.v22;
        case 43:
            return ((uint16_t)l.v32 << 16) | (uint16_t)l.v31;
        case 44:
            return (int32_t)l.v33;
        case 45:
            return bk.r;
        case 46:
            return bk.g;
        case 47:
            return bk.b;

        case 48:
            return ((uint16_t)lr.v12 << 16) | (uint16_t)lr.v11;
        case 49:
            return ((uint16_t)lr.v21 << 16) | (uint16_t)lr.v13;
        case 50:
            return ((uint16_t)lr.v23 << 16) | (uint16_t)lr.v22;
        case 51:
            return ((uint16_t)lr.v32 << 16) | (uint16_t)lr.v31;
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
            return (int32_t)dqa;
        case 60:
            return dqb;
        case 61:
            return (uint16_t)zsf3;
        case 62:
            return (uint16_t)zsf4;
        case 63:
            return flag;
        default:
            return 0;
    }
}

void GTE::write(uint8_t n, uint32_t d) {
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
            s[0].x = s[1].x;
            s[0].y = s[1].y;

            s[1].x = s[2].x;
            s[1].y = s[2].y;

            s[2].y = d >> 16;
            s[2].x = d;
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
            ir[1] = (d & 0x1f) * 0x80;
            ir[2] = ((d >> 5) & 0x1f) * 0x80;
            ir[3] = ((d >> 10) & 0x1f) * 0x80;
            break;
        case 29:
            break;
        case 30:
            lzcs = d;
            lzcr = countLeadingZeroes(lzcs);
            break;
        case 31:
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
    log.push_back({GTE_ENTRY::MODE::write, n, d});
}

int32_t GTE::clip(int32_t value, int32_t max, int32_t min, uint32_t flags) {
    if (value > max) {
        flag |= flags;
        return max;
    }
    if (value < min) {
        flag |= flags;
        return min;
    }
    return value;
}

void GTE::check43bitsOverflow(int64_t value, uint32_t overflowBits, uint32_t underflowFlags) {
    if (value > 0x7FFFFFFFFFFLL) flag |= overflowBits;
    if (value < -0x80000000000LL) flag |= underflowFlags;
}

int32_t GTE::A1(int64_t value, bool sf) {
    check43bitsOverflow(value, 1 << 31 | 1 << 30, 1 << 31 | 1 << 27);
    return value >> (sf * 12);
}

int32_t GTE::A2(int64_t value, bool sf) {
    check43bitsOverflow(value, 1 << 31 | 1 << 29, 1 << 31 | 1 << 26);
    return value >> (sf * 12);
}

int32_t GTE::A3(int64_t value, bool sf) {
    check43bitsOverflow(value, 1 << 31 | 1 << 28, 1 << 31 | 1 << 25);
    return value >> (sf * 12);
}

#define Lm_B1(x, lm) clip((x), 0x7fff, (lm) ? 0 : -0x8000, 1 << 31 | 1 << 24)
#define Lm_B2(x, lm) clip((x), 0x7fff, (lm) ? 0 : -0x8000, 1 << 31 | 1 << 23)
#define Lm_B3(x, lm) clip((x), 0x7fff, (lm) ? 0 : -0x8000, 1 << 22)

int32_t GTE::Lm_B3_sf(int64_t x, bool sf, bool lm) {
    Lm_B3(x >> 12, false);
    return clip(x >> (sf * 12), 0x7fff, lm ? 0 : -0x8000);
}

#define Lm_C1(x) (x)
#define Lm_C2(x) (x)
#define Lm_C3(x) (x)

#define Lm_D(x, sf) clip((x) >> ((1 - sf) * 12), 0xffff, 0x0000, 1 << 31 | 1 << 18)

#define Lm_G1(x) clip((x), 0x3ff, -0x400, 1 << 31 | 1 << 14)
#define Lm_G2(x) clip((x), 0x3ff, -0x400, 1 << 31 | 1 << 13)

int32_t GTE::F(int64_t value) {
    if (value > 0x7fffffffLL) flag |= 1 << 31 | 1 << 16;
    if (value < -0x80000000LL) flag |= 1 << 31 | 1 << 15;
    return value;
}

#define Lm_H(x) clip((x) >> 12, 0x1000, 0x0000, 1 << 12)

#define Lm_E(x) (x)

#define TRX (tr.x)
#define TRY (tr.y)
#define TRZ (tr.z)

#define R11 (rt.v11)
#define R12 (rt.v12)
#define R13 (rt.v13)

#define R21 (rt.v21)
#define R22 (rt.v22)
#define R23 (rt.v23)

#define R31 (rt.v31)
#define R32 (rt.v32)
#define R33 (rt.v33)

#define R (rgbc.read(0) << 4)
#define G (rgbc.read(1) << 4)
#define B (rgbc.read(2) << 4)
#define CODE (rgbc.read(3) << 4)

void GTE::nclip() { mac[0] = F(s[0].x * s[1].y + s[1].x * s[2].y + s[2].x * s[0].y - s[0].x * s[2].y - s[1].x * s[0].y - s[2].x * s[1].y); }

void GTE::ncds(bool sf, bool lm, int n) {
    // TODO
    mac[1] = A1(l.v11 * v[n].x + l.v12 * v[n].y + l.v13 * v[n].z);
    mac[2] = A2(l.v21 * v[n].x + l.v22 * v[n].y + l.v23 * v[n].z);
    mac[3] = A3(l.v31 * v[n].x + l.v32 * v[n].y + l.v33 * v[n].z);

    ir[1] = Lm_B1(mac[1], lm);
    ir[2] = Lm_B2(mac[2], lm);
    ir[3] = Lm_B3(mac[3], lm);

    mac[1] = A1((bk.r << 12) + lr.v11 * ir[1] + lr.v12 * ir[2] + lr.v13 * ir[3], sf);
    mac[2] = A2((bk.g << 12) + lr.v21 * ir[1] + lr.v22 * ir[2] + lr.v23 * ir[3], sf);
    mac[3] = A3((bk.b << 12) + lr.v31 * ir[1] + lr.v32 * ir[2] + lr.v33 * ir[3], sf);

    ir[1] = Lm_B1(mac[1], lm);
    ir[2] = Lm_B2(mac[2], lm);
    ir[3] = Lm_B3(mac[3], lm);

    mac[1] = A1(R * ir[1] + ir[0] * Lm_B1(A1((fc.r << 12) - R * ir[1]), 0));
    mac[2] = A2(G * ir[2] + ir[0] * Lm_B2(A2((fc.g << 12) - G * ir[2]), 0));
    mac[3] = A3(B * ir[3] + ir[0] * Lm_B3(A3((fc.b << 12) - B * ir[3]), 0));

    ir[1] = Lm_B1(mac[1], lm);
    ir[2] = Lm_B2(mac[2], lm);
    ir[3] = Lm_B3(mac[3], lm);

    rgb[0] = rgb[1];
    rgb[1] = rgb[2];

    rgb[2].write(0, Lm_C1(mac[1] >> 4));  // R
    rgb[2].write(1, Lm_C2(mac[2] >> 4));  // G
    rgb[2].write(2, Lm_C3(mac[3] >> 4));  // B
    rgb[2].write(3, CODE);                // CODE
}

void GTE::nccs(bool sf, bool lm, int n) {
    // TODO
    mac[1] = A1(l.v11 * v[n].x + l.v12 * v[n].y + l.v13 * v[n].z);
    mac[2] = A2(l.v21 * v[n].x + l.v22 * v[n].y + l.v23 * v[n].z);
    mac[3] = A3(l.v31 * v[n].x + l.v32 * v[n].y + l.v33 * v[n].z);

    ir[1] = Lm_B1(mac[1], lm);
    ir[2] = Lm_B2(mac[2], lm);
    ir[3] = Lm_B3(mac[3], lm);

    mac[1] = A1((bk.r << 12) + lr.v11 * ir[1] + lr.v12 * ir[2] + lr.v13 * ir[3], sf);
    mac[2] = A2((bk.g << 12) + lr.v21 * ir[1] + lr.v22 * ir[2] + lr.v23 * ir[3], sf);
    mac[3] = A3((bk.b << 12) + lr.v31 * ir[1] + lr.v32 * ir[2] + lr.v33 * ir[3], sf);

    ir[1] = Lm_B1(mac[1], lm);
    ir[2] = Lm_B2(mac[2], lm);
    ir[3] = Lm_B3(mac[3], lm);

    mac[1] = A1(R * ir[1]);
    mac[2] = A2(G * ir[2]);
    mac[3] = A3(B * ir[3]);

    ir[1] = Lm_B1(mac[1], lm);
    ir[2] = Lm_B2(mac[2], lm);
    ir[3] = Lm_B3(mac[3], lm);

    rgb[0] = rgb[1];
    rgb[1] = rgb[2];

    rgb[2].write(0, Lm_C1(mac[1] >> 4));  // R
    rgb[2].write(1, Lm_C2(mac[2] >> 4));  // G
    rgb[2].write(2, Lm_C3(mac[3] >> 4));  // B
    rgb[2].write(3, CODE);                // CODE
}

void GTE::ncdt(bool sf, bool lm) {
    ncds(sf, lm, 0);
    ncds(sf, lm, 1);
    ncds(sf, lm, 2);
}

void GTE::ncct(bool sf, bool lm) {
    nccs(sf, lm, 0);
    nccs(sf, lm, 1);
    nccs(sf, lm, 2);
}

void GTE::dcpt(bool sf, bool lm) {
    dcps(sf, lm);
    dcps(sf, lm);
    dcps(sf, lm);
}

void GTE::dcps(bool sf, bool lm) {
    mac[1] = A1((R << 12) + ir[0] * Lm_B1(A1((fc.r << 12) - (R << 12), sf), 0), sf);
    mac[2] = A2((G << 12) + ir[0] * Lm_B2(A2((fc.g << 12) - (G << 12), sf), 0), sf);
    mac[3] = A3((B << 12) + ir[0] * Lm_B3(A3((fc.b << 12) - (B << 12), sf), 0), sf);

    ir[1] = Lm_B1(mac[1], lm);
    ir[2] = Lm_B2(mac[2], lm);
    ir[3] = Lm_B3(mac[3], lm);

    rgb[0] = rgb[1];
    rgb[1] = rgb[2];

    rgb[2].write(0, Lm_C1(mac[1] >> 4));  // R
    rgb[2].write(1, Lm_C2(mac[2] >> 4));  // G
    rgb[2].write(2, Lm_C3(mac[3] >> 4));  // B
    rgb[2].write(3, CODE);                // CODE
}

void GTE::dcpl(bool sf, bool lm) {
    mac[1] = A1(R * ir[1] + ir[0] * Lm_B1(A1((fc.r << 12) - R * ir[1], sf), 0), sf);
    mac[2] = A2(G * ir[2] + ir[0] * Lm_B2(A2((fc.g << 12) - G * ir[2], sf), 0), sf);
    mac[3] = A3(B * ir[3] + ir[0] * Lm_B3(A3((fc.b << 12) - B * ir[3], sf), 0), sf);

    ir[1] = Lm_B1(mac[1], lm);
    ir[2] = Lm_B2(mac[2], lm);
    ir[3] = Lm_B3(mac[3], lm);

    rgb[0] = rgb[1];
    rgb[1] = rgb[2];

    rgb[2].write(0, Lm_C1(mac[1] >> 4));  // R
    rgb[2].write(1, Lm_C2(mac[2] >> 4));  // G
    rgb[2].write(2, Lm_C3(mac[3] >> 4));  // B
    rgb[2].write(3, CODE);                // CODE
}

void GTE::intpl(bool sf, bool lm) {
    mac[1] = A1((ir[1] << 12) + ir[0] * Lm_B1(A1((fc.r << 12) - (ir[1] << 12), sf), 0), sf);
    mac[2] = A2((ir[2] << 12) + ir[0] * Lm_B2(A2((fc.g << 12) - (ir[2] << 12), sf), 0), sf);
    mac[3] = A3((ir[3] << 12) + ir[0] * Lm_B3(A3((fc.b << 12) - (ir[3] << 12), sf), 0), sf);

    ir[1] = Lm_B1(mac[1], lm);
    ir[2] = Lm_B2(mac[2], lm);
    ir[3] = Lm_B3(mac[3], lm);

    rgb[0] = rgb[1];
    rgb[1] = rgb[2];

    rgb[2].write(0, Lm_C1(mac[1] >> 4));  // R
    rgb[2].write(1, Lm_C2(mac[2] >> 4));  // G
    rgb[2].write(2, Lm_C3(mac[3] >> 4));  // B
    rgb[2].write(3, CODE);                // CODE
}

int GTE::countLeadingZeroes(uint32_t n) {
    int zeroes = 0;
    if ((n & 0x80000000) == 0) n = ~n;

    while ((n & 0x80000000) != 0) {
        zeroes++;
        n <<= 1;
    }
    return zeroes;
}

template <typename T>
T min(T a, T b) {
    return (a < b) ? a : b;
}

// uint8_t unr_table(int i)
//{
//	return min(0, (0x40000 / (i + 0x100) + 1) / 2 - 0x101);
//}
static uint8_t unr_table[]
    = {0xFF, 0xFD, 0xFB, 0xF9, 0xF7, 0xF5, 0xF3, 0xF1, 0xEF, 0xEE, 0xEC, 0xEA, 0xE8, 0xE6, 0xE4, 0xE3, 0xE1, 0xDF, 0xDD, 0xDC, 0xDA, 0xD8,
       0xD6, 0xD5, 0xD3, 0xD1, 0xD0, 0xCE, 0xCD, 0xCB, 0xC9, 0xC8, 0xC6, 0xC5, 0xC3, 0xC1, 0xC0, 0xBE, 0xBD, 0xBB, 0xBA, 0xB8, 0xB7, 0xB5,
       0xB4, 0xB2, 0xB1, 0xB0, 0xAE, 0xAD, 0xAB, 0xAA, 0xA9, 0xA7, 0xA6, 0xA4, 0xA3, 0xA2, 0xA0, 0x9F, 0x9E, 0x9C, 0x9B, 0x9A, 0x99, 0x97,
       0x96, 0x95, 0x94, 0x92, 0x91, 0x90, 0x8F, 0x8D, 0x8C, 0x8B, 0x8A, 0x89, 0x87, 0x86, 0x85, 0x84, 0x83, 0x82, 0x81, 0x7F, 0x7E, 0x7D,
       0x7C, 0x7B, 0x7A, 0x79, 0x78, 0x77, 0x75, 0x74, 0x73, 0x72, 0x71, 0x70, 0x6F, 0x6E, 0x6D, 0x6C, 0x6B, 0x6A, 0x69, 0x68, 0x67, 0x66,
       0x65, 0x64, 0x63, 0x62, 0x61, 0x60, 0x5F, 0x5E, 0x5D, 0x5D, 0x5C, 0x5B, 0x5A, 0x59, 0x58, 0x57, 0x56, 0x55, 0x54, 0x53, 0x53, 0x52,
       0x51, 0x50, 0x4F, 0x4E, 0x4D, 0x4D, 0x4C, 0x4B, 0x4A, 0x49, 0x48, 0x48, 0x47, 0x46, 0x45, 0x44, 0x43, 0x43, 0x42, 0x41, 0x40, 0x3F,
       0x3F, 0x3E, 0x3D, 0x3C, 0x3C, 0x3B, 0x3A, 0x39, 0x39, 0x38, 0x37, 0x36, 0x36, 0x35, 0x34, 0x33, 0x33, 0x32, 0x31, 0x31, 0x30, 0x2F,
       0x2E, 0x2E, 0x2D, 0x2C, 0x2C, 0x2B, 0x2A, 0x2A, 0x29, 0x28, 0x28, 0x27, 0x26, 0x26, 0x25, 0x24, 0x24, 0x23, 0x22, 0x22, 0x21, 0x20,
       0x20, 0x1F, 0x1E, 0x1E, 0x1D, 0x1D, 0x1C, 0x1B, 0x1B, 0x1A, 0x19, 0x19, 0x18, 0x18, 0x17, 0x16, 0x16, 0x15, 0x15, 0x14, 0x14, 0x13,
       0x12, 0x12, 0x11, 0x11, 0x10, 0x0F, 0x0F, 0x0E, 0x0E, 0x0D, 0x0D, 0x0C, 0x0C, 0x0B, 0x0A, 0x0A, 0x09, 0x09, 0x08, 0x08, 0x07, 0x07,
       0x06, 0x06, 0x05, 0x05, 0x04, 0x04, 0x03, 0x03, 0x02, 0x02, 0x01, 0x01, 0x00, 0x00, 0x00};

int32_t GTE::divide(uint16_t h, uint16_t sz3) {
    //    if (h < sz3*2)
    //    {
    //		uint32_t z = countLeadingZeroes(sz3);
    //		uint32_t n = (h << z) & 0x7fff8000;
    //		uint32_t d = (sz3 << z) & 0xffff;
    //		uint32_t u = unr_table[(d + 0x40) >> 7] + 0x101;
    //		d = (0x2000080 - (d * u)) >> 8;
    //		d = (0x0000080 + (d * u)) >> 8;
    //		return min(0x1ffffu, ((n*d) + 0x8000) >> 16);
    //    }
    if (sz3 == 0) {
        flag |= (1 << 31) | (1 << 17);
        return 0x1ffff;
    }
    int32_t n = (((int64_t)h * 0x10000 + (int64_t)sz3 / 2) / (int64_t)sz3);
    if (n > 0x1ffff) {
        flag |= (1 << 31) | (1 << 17);
        return 0x1ffff;
    }
    return n;
}

void GTE::rtps(int n, bool sf, bool lm) {
    mac[1] = A1(TRX * 0x1000 + R11 * v[n].x + R12 * v[n].y + R13 * v[n].z, sf);
    mac[2] = A2(TRY * 0x1000 + R21 * v[n].x + R22 * v[n].y + R23 * v[n].z, sf);
    mac[3] = A3(TRZ * 0x1000 + R31 * v[n].x + R32 * v[n].y + R33 * v[n].z, sf);

    ir[1] = Lm_B1(mac[1], 0);
    ir[2] = Lm_B2(mac[2], 0);
    ir[3] = Lm_B3(mac[3], 0);

    s[0].z = s[1].z;
    s[1].z = s[2].z;
    s[2].z = s[3].z;
    s[3].z = Lm_D(mac[3], sf);

    int64_t h_s3z = divide(h, s[3].z);

    s[0].x = s[1].x;
    s[0].y = s[1].y;
    s[1].x = s[2].x;
    s[1].y = s[2].y;
    s[2].x = Lm_G1(F((int64_t)of[0] + ir[1] * h_s3z) >> 16);
    s[2].y = Lm_G2(F((int64_t)of[1] + ir[2] * h_s3z) >> 16);

    mac[0] = F(dqb + dqa * h_s3z);
    ir[0] = Lm_H(mac[0]);
}

void GTE::rtpt(bool sf, bool lm) {
    rtps(0, sf, lm);
    rtps(1, sf, lm);
    rtps(2, sf, lm);
}

void GTE::avsz3() {
    mac[0] = F(zsf3 * s[1].z + zsf3 * s[2].z + zsf3 * s[3].z);
    otz = Lm_D(mac[0], 0);
}

void GTE::avsz4() {
    mac[0] = F(zsf4 * s[0].z + zsf4 * s[1].z + zsf4 * s[2].z + zsf4 * s[3].z);
    otz = Lm_D(mac[0], 0);
}

void GTE::mvmva(bool sf, bool lm, int mx, int vx, int tx) {
    Matrix Mx;
    if (mx == 0)
        Mx = rt;
    else if (mx == 1)
        Mx = l;
    else if (mx == 2)
        Mx = lr;
    else
        printf("Invalid mvmva parameter: mx\n");

    Vector<int16_t> V;
    if (vx == 0) {
        V = v[0];
    } else if (vx == 1) {
        V = v[1];
    } else if (vx == 2) {
        V = v[2];
    } else {
        V.x = ir[1];
        V.y = ir[2];
        V.z = ir[3];
    }

    Vector<int32_t> Tx;
    if (tx == 0)
        Tx = tr;
    else if (tx == 1)
        Tx = bk;
    else if (tx == 2)
        Tx = fc;
    else
        Tx.x = Tx.y = Tx.z = 0;

    if (tx == 2) {
        mac[1] = A1(Mx.v12 * V.y + Mx.v13 * V.z, sf);
        mac[2] = A2(Mx.v22 * V.y + Mx.v23 * V.z, sf);
        mac[3] = A3(Mx.v32 * V.y + Mx.v33 * V.z, sf);
        Lm_B1(A1((Tx.x << 12) + Mx.v11 * V.x), 0);
        Lm_B2(A1((Tx.y << 12) + Mx.v21 * V.x), 0);
        Lm_B3(A1((Tx.z << 12) + Mx.v31 * V.x), 0);
    } else {
        mac[1] = A1((Tx.x * 0x1000) + Mx.v11 * V.x + Mx.v12 * V.y + Mx.v13 * V.z, sf);
        mac[2] = A2((Tx.y * 0x1000) + Mx.v21 * V.x + Mx.v22 * V.y + Mx.v23 * V.z, sf);
        mac[3] = A3((Tx.z * 0x1000) + Mx.v31 * V.x + Mx.v32 * V.y + Mx.v33 * V.z, sf);
    }

    ir[1] = Lm_B1(mac[1], lm);
    ir[2] = Lm_B2(mac[2], lm);
    ir[3] = Lm_B3(mac[3], lm);
}

void GTE::gpf(bool sf, bool lm) {
    mac[1] = A1(ir[0] * ir[1], sf);
    mac[2] = A2(ir[0] * ir[2], sf);
    mac[3] = A3(ir[0] * ir[3], sf);

    ir[1] = Lm_B1(mac[1], lm);
    ir[2] = Lm_B2(mac[2], lm);
    ir[3] = Lm_B3(mac[3], lm);

    rgb[0] = rgb[1];
    rgb[1] = rgb[2];

    rgb[2].write(0, Lm_C1(mac[1] >> 4));  // R
    rgb[2].write(1, Lm_C2(mac[2] >> 4));  // G
    rgb[2].write(2, Lm_C3(mac[3] >> 4));  // B
    rgb[2].write(3, CODE);                // CODE
}

void GTE::gpl(bool sf, bool lm) {
    mac[1] = A1(mac[1] + ir[0] * ir[1], sf);
    mac[2] = A2(mac[2] + ir[0] * ir[2], sf);
    mac[3] = A3(mac[3] + ir[0] * ir[3], sf);

    ir[1] = Lm_B1(mac[1], lm);
    ir[2] = Lm_B2(mac[2], lm);
    ir[3] = Lm_B3(mac[3], lm);

    rgb[0] = rgb[1];
    rgb[1] = rgb[2];

    rgb[2].write(0, Lm_C1(mac[1] >> 4));  // R
    rgb[2].write(1, Lm_C2(mac[2] >> 4));  // G
    rgb[2].write(2, Lm_C3(mac[3] >> 4));  // B
    rgb[2].write(3, CODE);                // CODE
}

void GTE::sqr(bool sf, bool lm) {
    mac[1] = A1(ir[1] * ir[1], sf);
    mac[2] = A2(ir[2] * ir[2], sf);
    mac[3] = A3(ir[3] * ir[3], sf);

    ir[1] = Lm_B1(mac[1], lm);
    ir[2] = Lm_B2(mac[2], lm);
    ir[3] = Lm_B3(mac[3], lm);
}

void GTE::op(bool sf, bool lm) {
    int32_t d1 = R11 * R12;
    int32_t d2 = R22 * R23;
    int32_t d3 = R33;

    mac[1] = A1(d2 * ir[3] - d3 * ir[2], sf);
    mac[2] = A2(d3 * ir[1] - d1 * ir[3], sf);
    mac[3] = A3(d1 * ir[2] - d2 * ir[1], sf);

    ir[1] = Lm_B1(mac[1], lm);
    ir[2] = Lm_B2(mac[2], lm);
    ir[3] = Lm_B3(mac[3], lm);
}
}
}
