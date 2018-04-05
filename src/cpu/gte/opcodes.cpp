#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdio>
#include "gte.h"

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
    return (int32_t)(value >> (sf * 12));
}

int32_t GTE::A2(int64_t value, bool sf) {
    check43bitsOverflow(value, 1 << 31 | 1 << 29, 1 << 31 | 1 << 26);
    return (int32_t)(value >> (sf * 12));
}

int32_t GTE::A3(int64_t value, bool sf) {
    check43bitsOverflow(value, 1 << 31 | 1 << 28, 1 << 31 | 1 << 25);
    return (int32_t)(value >> (sf * 12));
}

#define Lm_B1(x, lm) clip((x), 0x7fff, (lm) ? 0 : -0x8000, 1 << 31 | 1 << 24)
#define Lm_B2(x, lm) clip((x), 0x7fff, (lm) ? 0 : -0x8000, 1 << 31 | 1 << 23)
#define Lm_B3(x, lm) clip((x), 0x7fff, (lm) ? 0 : -0x8000, 1 << 22)

#define Lm_D(x, sf) clip((x) >> ((1 - sf) * 12), 0xffff, 0x0000, 1 << 31 | 1 << 18)

int32_t GTE::F(int64_t value) {
    if (value > 0x7fffffffLL) flag |= 1 << 31 | 1 << 16;
    if (value < -0x80000000LL) flag |= 1 << 31 | 1 << 15;
    return (int32_t)value;
}

int64_t GTE::setMac(int i, int64_t value) {
    assert(i >= 0 && i <= 3);
    uint32_t overflowBits = 0;
    uint32_t underflowBits = 0;

    if (i == 0) {
        overflowBits |= 1 << 16;
        underflowBits |= 1 << 15;

        if (value > 0x7fffffffLL) flag |= overflowBits;
        if (value < -0x80000000LL) flag |= underflowBits;

        mac[0] = (int32_t)value;
        return value;
    } else if (i == 1) {
        overflowBits |= 1 << 30;
        underflowBits |= 1 << 27;
    } else if (i == 2) {
        overflowBits |= 1 << 29;
        underflowBits |= 1 << 26;
    } else if (i == 3) {
        overflowBits |= 1 << 28;
        underflowBits |= 1 << 25;
    }

    check43bitsOverflow(value, overflowBits, underflowBits);

    if (sf) value >>= 12;
    mac[i] = (int32_t)value;
    return value;
}

// clang-format off
void GTE::setIr(int i, int64_t value, bool lm) {
    if (i == 0) {
        value >>= 12;
        ir[0] = clip(value, 0x1000, 0x0000, 1 << 12);
        return;
    }

    uint32_t saturatedBits = 0;
    if (i == 1) saturatedBits = 1 << 24;
    else if (i == 2) saturatedBits = 1 << 23;
    else if (i == 3) saturatedBits = 1 << 22;

    if (lm) ir[i] = clip(value, 0x7fff, 0x0000, saturatedBits);
    else    ir[i] = clip(value, 0x7fff, -0x8000, saturatedBits);
}
// clang-format on

void GTE::setMacAndIr(int i, int64_t value, bool lm) { setIr(i, setMac(i, value), lm); }

void GTE::setOtz(int32_t value) {
    value >>= 12;
    otz = clip(value, 0xffff, 0x0000, 1 << 18);
}

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
#define CODE (rgbc.read(3))

void GTE::nclip() { setMac(0, s[0].x * s[1].y + s[1].x * s[2].y + s[2].x * s[0].y - s[0].x * s[2].y - s[1].x * s[0].y - s[2].x * s[1].y); }

void GTE::ncds(bool sf, bool lm, int n) {
    mac[1] = A1(l.v11 * v[n].x + l.v12 * v[n].y + l.v13 * v[n].z, sf);
    mac[2] = A2(l.v21 * v[n].x + l.v22 * v[n].y + l.v23 * v[n].z, sf);
    mac[3] = A3(l.v31 * v[n].x + l.v32 * v[n].y + l.v33 * v[n].z, sf);

    ir[1] = Lm_B1(mac[1], lm);
    ir[2] = Lm_B2(mac[2], lm);
    ir[3] = Lm_B3(mac[3], lm);

    mac[1] = A1((bk.r * 0x1000) + lr.v11 * ir[1] + lr.v12 * ir[2] + lr.v13 * ir[3], sf);
    mac[2] = A2((bk.g * 0x1000) + lr.v21 * ir[1] + lr.v22 * ir[2] + lr.v23 * ir[3], sf);
    mac[3] = A3((bk.b * 0x1000) + lr.v31 * ir[1] + lr.v32 * ir[2] + lr.v33 * ir[3], sf);

    ir[1] = Lm_B1(mac[1], lm);
    ir[2] = Lm_B2(mac[2], lm);
    ir[3] = Lm_B3(mac[3], lm);

    mac[1] = A1((R << 12) + ir[0] * Lm_B1(A1((fc.r << 12) - (R << 12), sf), 0), sf);
    mac[2] = A2((G << 12) + ir[0] * Lm_B2(A2((fc.g << 12) - (G << 12), sf), 0), sf);
    mac[3] = A3((B << 12) + ir[0] * Lm_B3(A3((fc.b << 12) - (B << 12), sf), 0), sf);

    ir[1] = Lm_B1(mac[1], lm);
    ir[2] = Lm_B2(mac[2], lm);
    ir[3] = Lm_B3(mac[3], lm);

    pushColor(mac[1] / 16, mac[2] / 16, mac[3] / 16);
}

void GTE::nccs(bool sf, bool lm, int n) {
    mac[1] = A1(l.v11 * v[n].x + l.v12 * v[n].y + l.v13 * v[n].z, sf);
    mac[2] = A2(l.v21 * v[n].x + l.v22 * v[n].y + l.v23 * v[n].z, sf);
    mac[3] = A3(l.v31 * v[n].x + l.v32 * v[n].y + l.v33 * v[n].z, sf);

    ir[1] = Lm_B1(mac[1], lm);
    ir[2] = Lm_B2(mac[2], lm);
    ir[3] = Lm_B3(mac[3], lm);

    mac[1] = A1((bk.r * 0x1000) + lr.v11 * ir[1] + lr.v12 * ir[2] + lr.v13 * ir[3], sf);
    mac[2] = A2((bk.g * 0x1000) + lr.v21 * ir[1] + lr.v22 * ir[2] + lr.v23 * ir[3], sf);
    mac[3] = A3((bk.b * 0x1000) + lr.v31 * ir[1] + lr.v32 * ir[2] + lr.v33 * ir[3], sf);

    ir[1] = Lm_B1(mac[1], lm);
    ir[2] = Lm_B2(mac[2], lm);
    ir[3] = Lm_B3(mac[3], lm);

    mac[1] = A1(R * ir[1], sf);
    mac[2] = A2(G * ir[2], sf);
    mac[3] = A3(B * ir[3], sf);

    ir[1] = Lm_B1(mac[1], lm);
    ir[2] = Lm_B2(mac[2], lm);
    ir[3] = Lm_B3(mac[3], lm);

    pushColor(mac[1] / 16, mac[2] / 16, mac[3] / 16);
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

    pushColor(mac[1] / 16, mac[2] / 16, mac[3] / 16);
}

void GTE::dcpl(bool sf, bool lm) {
    mac[1] = A1(R * ir[1] + ir[0] * Lm_B1(A1((fc.r << 12) - R * ir[1], sf), 0), sf);
    mac[2] = A2(G * ir[2] + ir[0] * Lm_B2(A2((fc.g << 12) - G * ir[2], sf), 0), sf);
    mac[3] = A3(B * ir[3] + ir[0] * Lm_B3(A3((fc.b << 12) - B * ir[3], sf), 0), sf);

    ir[1] = Lm_B1(mac[1], lm);
    ir[2] = Lm_B2(mac[2], lm);
    ir[3] = Lm_B3(mac[3], lm);

    pushColor(mac[1] / 16, mac[2] / 16, mac[3] / 16);
}

void GTE::intpl(bool sf, bool lm) {
    mac[1] = A1((ir[1] << 12) + ir[0] * Lm_B1(A1((fc.r << 12) - (ir[1] << 12), sf), 0), sf);
    mac[2] = A2((ir[2] << 12) + ir[0] * Lm_B2(A2((fc.g << 12) - (ir[2] << 12), sf), 0), sf);
    mac[3] = A3((ir[3] << 12) + ir[0] * Lm_B3(A3((fc.b << 12) - (ir[3] << 12), sf), 0), sf);

    ir[1] = Lm_B1(mac[1], lm);
    ir[2] = Lm_B2(mac[2], lm);
    ir[3] = Lm_B3(mac[3], lm);

    pushColor(mac[1] / 16, mac[2] / 16, mac[3] / 16);
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

size_t GTE::countLeadingZeroes16(uint16_t n) {
    size_t zeroes = 0;

    while (!(n & 0x8000) && zeroes < 16) {
        zeroes++;
        n <<= 1;
    }
    return zeroes;
}

uint32_t GTE::divide(uint16_t h, uint16_t sz3) {
    if (sz3 == 0) {
        flag |= 1 << 17;
        return 0x1ffff;
    }
    uint64_t n = (((uint64_t)h * 0x20000 / (uint64_t)sz3) + 1) / 2;
    if (n > 0x1ffff) {
        flag |= 1 << 17;
        return 0x1ffff;
    }
    return n;
}

uint8_t unr_table[0x101]
    = {0xff, 0xfd, 0xfb, 0xf9, 0xf7, 0xf5, 0xf3, 0xf1, 0xef, 0xee, 0xec, 0xea, 0xe8, 0xe6, 0xe4, 0xe3, 0xe1, 0xdf, 0xdd, 0xdc, 0xda, 0xd8,
       0xd6, 0xd5, 0xd3, 0xd1, 0xd0, 0xce, 0xcd, 0xcb, 0xc9, 0xc8, 0xc6, 0xc5, 0xc3, 0xc1, 0xc0, 0xbe, 0xbd, 0xbb, 0xba, 0xb8, 0xb7, 0xb5,
       0xb4, 0xb2, 0xb1, 0xb0, 0xae, 0xad, 0xab, 0xaa, 0xa9, 0xa7, 0xa6, 0xa4, 0xa3, 0xa2, 0xa0, 0x9f, 0x9e, 0x9c, 0x9b, 0x9a, 0x99, 0x97,
       0x96, 0x95, 0x94, 0x92, 0x91, 0x90, 0x8f, 0x8d, 0x8c, 0x8b, 0x8a, 0x89, 0x87, 0x86, 0x85, 0x84, 0x83, 0x82, 0x81, 0x7f, 0x7e, 0x7d,
       0x7c, 0x7b, 0x7a, 0x79, 0x78, 0x77, 0x75, 0x74, 0x73, 0x72, 0x71, 0x70, 0x6f, 0x6e, 0x6d, 0x6c, 0x6b, 0x6a, 0x69, 0x68, 0x67, 0x66,
       0x65, 0x64, 0x63, 0x62, 0x61, 0x60, 0x5f, 0x5e, 0x5d, 0x5d, 0x5c, 0x5b, 0x5a, 0x59, 0x58, 0x57, 0x56, 0x55, 0x54, 0x53, 0x53, 0x52,
       0x51, 0x50, 0x4f, 0x4e, 0x4d, 0x4d, 0x4c, 0x4b, 0x4a, 0x49, 0x48, 0x48, 0x47, 0x46, 0x45, 0x44, 0x43, 0x43, 0x42, 0x41, 0x40, 0x3f,
       0x3f, 0x3e, 0x3d, 0x3c, 0x3c, 0x3b, 0x3a, 0x39, 0x39, 0x38, 0x37, 0x36, 0x36, 0x35, 0x34, 0x33, 0x33, 0x32, 0x31, 0x31, 0x30, 0x2f,
       0x2e, 0x2e, 0x2d, 0x2c, 0x2c, 0x2b, 0x2a, 0x2a, 0x29, 0x28, 0x28, 0x27, 0x26, 0x26, 0x25, 0x24, 0x24, 0x23, 0x22, 0x22, 0x21, 0x20,
       0x20, 0x1f, 0x1e, 0x1e, 0x1d, 0x1d, 0x1c, 0x1b, 0x1b, 0x1a, 0x19, 0x19, 0x18, 0x18, 0x17, 0x16, 0x16, 0x15, 0x15, 0x14, 0x14, 0x13,
       0x12, 0x12, 0x11, 0x11, 0x10, 0x0f, 0x0f, 0x0e, 0x0e, 0x0d, 0x0d, 0x0c, 0x0c, 0x0b, 0x0a, 0x0a, 0x09, 0x09, 0x08, 0x08, 0x07, 0x07,
       0x06, 0x06, 0x05, 0x05, 0x04, 0x04, 0x03, 0x03, 0x02, 0x02, 0x01, 0x01, 0x00, 0x00, 0x00};

uint32_t recip(uint16_t divisor) {
    int32_t x = 0x101 + unr_table[((divisor & 0x7fff) + 0x40) >> 7];

    int32_t tmp = (((int32_t)divisor * -x) + 0x80) >> 8;
    int32_t tmp2 = ((x * (131072 + tmp)) + 0x80) >> 8;

    return tmp2;
}

uint32_t GTE::divideUNR(uint32_t lhs, uint32_t rhs) {
    if (!(rhs * 2 > lhs)) {
        flag |= 1 << 17;
        return 0x1ffff;
    }

    size_t shift = countLeadingZeroes16(rhs);
    lhs <<= shift;
    rhs <<= shift;

    uint32_t reciprocal = recip(rhs | 0x8000);

    uint32_t result = ((uint64_t)lhs * reciprocal + 0x8000) >> 16;

    if (result > 0x1ffff) {
        flag |= 1 << 17;
        return 0x1ffff;
    }
    return result;
}

void GTE::pushScreenXY(int32_t x, int32_t y) {
    s[0].x = s[1].x;
    s[0].y = s[1].y;
    s[1].x = s[2].x;
    s[1].y = s[2].y;

    s[2].x = clip(x, 0x3ff, -0x400, 1 << 14);
    s[2].y = clip(y, 0x3ff, -0x400, 1 << 13);
}

void GTE::pushScreenZ(int32_t z) {
    s[0].z = s[1].z;
    s[1].z = s[2].z;
    s[2].z = s[3].z;  // There is only s[3].z (no s[3].xy)

    s[3].z = clip(z, 0xffff, 0x0000, 1 << 18);
}

void GTE::pushColor(uint32_t r, uint32_t g, uint32_t b) {
    rgb[0] = rgb[1];
    rgb[1] = rgb[2];

    rgb[2].write(0, clip(r, 0xff, 0x00, 1 << 21));
    rgb[2].write(1, clip(g, 0xff, 0x00, 1 << 20));
    rgb[2].write(2, clip(b, 0xff, 0x00, 1 << 19));
    rgb[2].write(3, rgbc.read(3));
}

/**
 * Rotate, translate and perspective transformation
 *
 * Multiplicate vector (V) with rotation matrix (R),
 * translate it (TR) and apply perspective transformation.
 */
void GTE::rtps(int n) {
    setMacAndIr(1, (int64_t)tr.x * 0x1000 + R11 * v[n].x + R12 * v[n].y + R13 * v[n].z, lm);
    setMacAndIr(2, (int64_t)tr.y * 0x1000 + R21 * v[n].x + R22 * v[n].y + R23 * v[n].z, lm);
    int64_t mac3 = (int64_t)tr.z * 0x1000 + R31 * v[n].x + R32 * v[n].y + R33 * v[n].z;
    setMacAndIr(3, mac3, lm);

    pushScreenZ(mac3 >> 12);
    int64_t h_s3z = divideUNR(h, s[3].z);

    int32_t x = setMac(0, h_s3z * ir[1] + of[0]) >> 16;
    int32_t y = setMac(0, h_s3z * ir[2] + of[1]) >> 16;
    pushScreenXY(x, y);

    setMacAndIr(0, h_s3z * dqa + dqb, lm);
}

/**
 * Same as RTPS, but repeated for vector 0, 1 and 2
 */
void GTE::rtpt() {
    rtps(0);
    rtps(1);
    rtps(2);
}

void GTE::avsz3() {
    setMac(0, (int64_t)zsf3 * (s[1].z + s[2].z + s[3].z));
    setOtz(mac[0]);
}

void GTE::avsz4() {
    setMac(0, (int64_t)zsf4 * (s[0].z + s[1].z + s[2].z + s[3].z));
    setOtz(mac[0]);
}

void GTE::mvmva(bool sf, bool lm, int mx, int vx, int tx) {
    gte::Matrix Mx;
    if (mx == 0)
        Mx = rt;
    else if (mx == 1)
        Mx = l;
    else if (mx == 2)
        Mx = lr;
    else
        printf("Invalid mvmva parameter: mx\n");

    gte::Vector<int16_t> V;
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

    gte::Vector<int32_t> Tx;
    if (tx == 0)
        Tx = tr;
    else if (tx == 1)
        Tx = bk;
    else if (tx == 2) {
        Tx = fc;
        printf("Bugged mvmva parameter: tx == 2\n");
    } else
        Tx.x = Tx.y = Tx.z = 0;

    if (tx == 2) {
        setMacAndIr(1, Mx.v12 * V.y + Mx.v13 * V.z, lm);
        setMacAndIr(2, Mx.v22 * V.y + Mx.v23 * V.z, lm);
        setMacAndIr(3, Mx.v32 * V.y + Mx.v33 * V.z, lm);
        //        Lm_B1(A1((Tx.x << 12) + Mx.v11 * V.x), 0);
        //        Lm_B2(A1((Tx.y << 12) + Mx.v21 * V.x), 0);
        //        Lm_B3(A1((Tx.z << 12) + Mx.v31 * V.x), 0);
    } else {
        setMacAndIr(1, Tx.x * 0x1000 + Mx.v11 * V.x + Mx.v12 * V.y + Mx.v13 * V.z, lm);
        setMacAndIr(2, Tx.y * 0x1000 + Mx.v21 * V.x + Mx.v22 * V.y + Mx.v23 * V.z, lm);
        setMacAndIr(3, Tx.z * 0x1000 + Mx.v31 * V.x + Mx.v32 * V.y + Mx.v33 * V.z, lm);
    }
}

/**
 * General purpose interpolation
 * Multiply vector (ir[1..3]) by scalar(ir[0])
 *
 * Result is also saved as 24bit color
 */
void GTE::gpf(bool lm) {
    setMacAndIr(1, ir[0] * ir[1], lm);
    setMacAndIr(2, ir[0] * ir[2], lm);
    setMacAndIr(3, ir[0] * ir[3], lm);
    pushColor(mac[1] / 16, mac[2] / 16, mac[3] / 16);
}

// TODO: Check with docs and refactor
// TODO: Remove sf * 10
void GTE::gpl(bool sf, bool lm) {
    setMac(1, mac[1] << (sf * 12));
    setMac(2, mac[2] << (sf * 12));
    setMac(3, mac[3] << (sf * 12));
    setMacAndIr(1, ir[0] * ir[1] + mac[1], lm);
    setMacAndIr(2, ir[0] * ir[2] + mac[2], lm);
    setMacAndIr(3, ir[0] * ir[3] + mac[3], lm);

    pushColor(mac[1] / 16, mac[2] / 16, mac[3] / 16);
}

/**
 * Square vector
 * lm is ignored, as result cannot be negative
 */
void GTE::sqr() {
    setMacAndIr(1, ir[1] * ir[1]);
    setMacAndIr(2, ir[2] * ir[2]);
    setMacAndIr(3, ir[3] * ir[3]);
}

void GTE::op(bool sf, bool lm) {
    setMac(1, R22 * ir[3] - R33 * ir[2]);
    setMac(2, R33 * ir[1] - R11 * ir[3]);
    setMac(3, R11 * ir[2] - R22 * ir[1]);

    setIr(1, mac[1], lm);
    setIr(2, mac[2], lm);
    setIr(3, mac[3], lm);
}