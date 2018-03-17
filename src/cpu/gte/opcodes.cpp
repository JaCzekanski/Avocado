#include <cassert>
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
    uint32_t overflowBits = 1 << 31;
    uint32_t underflowBits = 1 << 31;

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

    if (sf) value /= 0x1000;
    mac[i] = (int32_t)value;
    return value;
}

// clang-format off
void GTE::setMacAndIr(int i, int64_t value, bool lm) {
    int32_t valMac = setMac(i, value);

    if (i == 0) {
        valMac /= 0x1000;
        ir[0] = clip(valMac, 0x1000, 0x0000, 1 << 12);
        return;
    }

    uint32_t saturatedBits = 0;
    if      (i == 1) saturatedBits = 1 << 24 | 1 << 31;
    else if (i == 2) saturatedBits = 1 << 23 | 1 << 31;
    else if (i == 3) saturatedBits = 1 << 22;
                
    if (lm) ir[i] = clip(valMac, 0x7fff,  0x0000, saturatedBits);
    else    ir[i] = clip(valMac, 0x7fff, -0x8000, saturatedBits);
}
// clang-format on

void GTE::setOtz(int32_t value) {
    value /= 0x1000;
    otz = clip(value, 0xffff, 0x0000, 1 << 18 | 1 << 31);
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

int32_t GTE::divide(uint16_t h, uint16_t sz3) {
    if (sz3 == 0) {
        flag |= (1 << 31) | (1 << 17);
        return 0x1ffff;
    }
    int32_t n = (h * 0x10000 + sz3 / 2) / sz3;
    if (n > 0x1ffff) {
        flag |= (1 << 31) | (1 << 17);
        return 0x1ffff;
    }
    return n;
}

void GTE::pushScreenXY(int16_t x, int16_t y) {
    s[0].x = s[1].x;
    s[0].y = s[1].y;
    s[1].x = s[2].x;
    s[1].y = s[2].y;

    s[2].x = clip(x, 0x3ff, -0x400, 1 << 14 | 1 << 31);
    s[2].y = clip(y, 0x3ff, -0x400, 1 << 13 | 1 << 31);
}

void GTE::pushScreenZ(uint32_t z) {
    s[0].z = s[1].z;
    s[1].z = s[2].z;
    s[2].z = s[3].z;  // There is only s[3].z (no s[3].xy)

    s[3].z = clip(z, 0xffff, 0x0000, 1 << 18 | 1 << 31);
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
 *
 * lm is ignored - treat like 0
 */
// clang-format off
void GTE::rtps(int n) {
    setMacAndIr(1, tr.x * 0x1000 + R11 * v[n].x + R12 * v[n].y + R13 * v[n].z);
    setMacAndIr(2, tr.y * 0x1000 + R21 * v[n].x + R22 * v[n].y + R23 * v[n].z);
    setMacAndIr(3, tr.z * 0x1000 + R31 * v[n].x + R32 * v[n].y + R33 * v[n].z);

    // Z (mac[3]) that is shifted 12 bits regardless of sf bit
    int32_t zShifted = sf ? mac[3] : mac[3] >> 12;
    pushScreenZ(zShifted);
    int64_t h_s3z = divide(h, s[3].z);

    pushScreenXY(
        setMac(0, h_s3z * ir[1] + of[0]) >> 16, 
        setMac(0, h_s3z * ir[2] + of[1]) >> 16
    );

    setMacAndIr(0, h_s3z * dqa + dqb);
}
// clang-format on

/**
 * Same as RTPS, but repeated for vector 0, 1 and 2
 */
void GTE::rtpt() {
    rtps(0);
    rtps(1);
    rtps(2);
}

void GTE::avsz3() {
    setMac(0, zsf3 * s[1].z + zsf3 * s[2].z + zsf3 * s[3].z);
    setOtz(mac[0]);
}

void GTE::avsz4() {
    setMac(0, zsf4 * s[0].z + zsf4 * s[1].z + zsf4 * s[2].z + zsf4 * s[3].z);
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
        mac[1] = A1(Mx.v12 * V.y + Mx.v13 * V.z, sf);
        mac[2] = A2(Mx.v22 * V.y + Mx.v23 * V.z, sf);
        mac[3] = A3(Mx.v32 * V.y + Mx.v33 * V.z, sf);
        Lm_B1(A1((Tx.x << 12) + Mx.v11 * V.x), 0);
        Lm_B2(A1((Tx.y << 12) + Mx.v21 * V.x), 0);
        Lm_B3(A1((Tx.z << 12) + Mx.v31 * V.x), 0);
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
    mac[1] = A1(R22 * ir[3] - R33 * ir[2], sf);
    mac[2] = A2(R33 * ir[1] - R11 * ir[3], sf);
    mac[3] = A3(R11 * ir[2] - R22 * ir[1], sf);

    ir[1] = Lm_B1(mac[1], lm);
    ir[2] = Lm_B2(mac[2], lm);
    ir[3] = Lm_B3(mac[3], lm);
}