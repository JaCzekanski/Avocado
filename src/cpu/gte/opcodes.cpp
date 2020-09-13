#include "gte.h"
#include "utils/screenshot.h"
#include <algorithm>

using gte::Matrix;
using gte::toVector;
using gte::Vector;

int32_t GTE::clip(int32_t value, int32_t max, int32_t min, uint32_t flags) {
    if (value > max) {
        flag.reg |= flags;
        return max;
    }
    if (value < min) {
        flag.reg |= flags;
        return min;
    }
    return value;
}

template <int bit_size>
void GTE::checkOverflow(int64_t value, uint32_t overflowBits, uint32_t underflowFlags) {
    if (value >= (1LL << (bit_size - 1))) flag.reg |= overflowBits;
    if (value < -(1LL << (bit_size - 1))) flag.reg |= underflowFlags;
}

template <int i>
int64_t GTE::checkMacOverflowAndExtend(int64_t value) {
    static_assert(i >= 1 && i <= 3, "Invalid mac for GTE::checkMacOverflowAndExtend");

    if (i == 1) {
        checkOverflow<44>(value, Flag::MAC1_OVERFLOW_POSITIVE, Flag::MAC1_OVERFLOW_NEGATIVE);
    } else if (i == 2) {
        checkOverflow<44>(value, Flag::MAC2_OVERFLOW_POSITIVE, Flag::MAC2_OVERFLOW_NEGATIVE);
    } else if (i == 3) {
        checkOverflow<44>(value, Flag::MAC3_OVERFLOW_POSITIVE, Flag::MAC3_OVERFLOW_NEGATIVE);
    }

    return extend_sign<44, int64_t>(value);
}

#define O(i, value) checkMacOverflowAndExtend<i>(value)

template <>
int64_t GTE::setMac<0>(int64_t value) {
    checkOverflow<32>(value, Flag::MAC0_OVERFLOW_POSITIVE, Flag::MAC0_OVERFLOW_NEGATIVE);
    mac[0] = value;
    return value;
}

template <int i>
int64_t GTE::setMac(int64_t value) {
    static_assert(i >= 1 && i <= 3, "Invalid mac for GTE::setMac");

    if (i == 1) {
        checkOverflow<44>(value, Flag::MAC1_OVERFLOW_POSITIVE, Flag::MAC1_OVERFLOW_NEGATIVE);
    } else if (i == 2) {
        checkOverflow<44>(value, Flag::MAC2_OVERFLOW_POSITIVE, Flag::MAC2_OVERFLOW_NEGATIVE);
    } else if (i == 3) {
        checkOverflow<44>(value, Flag::MAC3_OVERFLOW_POSITIVE, Flag::MAC3_OVERFLOW_NEGATIVE);
    }

    if (sf) value >>= 12;

    mac[i] = value;
    return value;
}

template <int i>
void GTE::setIr(int32_t value, bool lm) {
    static_assert(i >= 1 && i <= 3, "Invalid ir for GTE::setIr");

    uint32_t saturatedBits = 0;
    if (i == 1) {
        saturatedBits = Flag::IR1_SATURATED;
    } else if (i == 2) {
        saturatedBits = Flag::IR2_SATURATED;
    } else if (i == 3) {
        saturatedBits = Flag::IR3_SATURATED;
    }

    ir[i] = clip(value, 0x7fff, lm ? 0 : -0x8000, saturatedBits);
}

template <int i>
void GTE::setMacAndIr(int64_t value, bool lm) {
    setIr<i>(setMac<i>(value), lm);
}

void GTE::setOtz(int64_t value) { otz = clip(value >> 12, 0xffff, 0x0000, Flag::SZ3_OTZ_SATURATED); }

#define R (rgbc.read(0) << 4)
#define G (rgbc.read(1) << 4)
#define B (rgbc.read(2) << 4)

void GTE::nclip() {
    int64_t value = (int64_t)s[0].x * s[1].y + s[1].x * s[2].y + s[2].x * s[0].y - s[0].x * s[2].y - s[1].x * s[0].y - s[2].x * s[1].y;
    Screenshot *screenshot = screenshot->getInstance();
    // if (screenshot->getEnabled()) {
    //        screenshot->originalnclip = value;
    if (screenshot->doubleSided) {
        value = std::abs(value);
    }
    //}
    setMac<0>(value);
}

void GTE::multiplyVectors(Vector<int16_t> v1, Vector<int16_t> v2, Vector<int16_t> tr) {
    setMacAndIr<1>(((int64_t)tr.x << 12) + v1.x * v2.x, lm);
    setMacAndIr<2>(((int64_t)tr.y << 12) + v1.y * v2.y, lm);
    setMacAndIr<3>(((int64_t)tr.z << 12) + v1.z * v2.z, lm);
}

void GTE::multiplyMatrixByVector(Matrix m, Vector<int16_t> v, Vector<int32_t> tr) {
    Vector<int64_t> result;

    result.x = O(1, O(1, O(1, ((int64_t)tr.x << 12) + m[0][0] * v.x) + m[0][1] * v.y) + m[0][2] * v.z);
    result.y = O(2, O(2, O(2, ((int64_t)tr.y << 12) + m[1][0] * v.x) + m[1][1] * v.y) + m[1][2] * v.z);
    result.z = O(3, O(3, O(3, ((int64_t)tr.z << 12) + m[2][0] * v.x) + m[2][1] * v.y) + m[2][2] * v.z);

    setMacAndIr<1>(result.x, lm);
    setMacAndIr<2>(result.y, lm);
    setMacAndIr<3>(result.z, lm);
}

int64_t GTE::multiplyMatrixByVectorRTP(Matrix m, Vector<int16_t> v, Vector<int32_t> tr) {
    Vector<int64_t> result;

    result.x = O(1, O(1, O(1, ((int64_t)tr.x << 12) + m[0][0] * v.x) + m[0][1] * v.y) + m[0][2] * v.z);
    result.y = O(2, O(2, O(2, ((int64_t)tr.y << 12) + m[1][0] * v.x) + m[1][1] * v.y) + m[1][2] * v.z);
    result.z = O(3, O(3, O(3, ((int64_t)tr.z << 12) + m[2][0] * v.x) + m[2][1] * v.y) + m[2][2] * v.z);

    setMacAndIr<1>(result.x, lm);
    setMacAndIr<2>(result.y, lm);
    setMac<3>(result.z);

    // RTP calculates IR3 saturation flag as if lm bit was always false
    clip(result.z >> 12, 0x7fff, -0x8000, Flag::IR3_SATURATED);

    // But calculation itself respects lm bit
    ir[3] = clip(mac[3], 0x7fff, lm ? 0 : -0x8000);

    return result.z;
}

void GTE::ncds(int n) {
    multiplyMatrixByVector(light, v[n]);
    multiplyMatrixByVector(color, toVector(ir), backgroundColor);

    auto prevIr = toVector(ir);

    setMacAndIr<1>(((int64_t)farColor.r << 12) - (R * ir[1]));
    setMacAndIr<2>(((int64_t)farColor.g << 12) - (G * ir[2]));
    setMacAndIr<3>(((int64_t)farColor.b << 12) - (B * ir[3]));

    setMacAndIr<1>((R * prevIr.x) + ir[0] * ir[1], lm);
    setMacAndIr<2>((G * prevIr.y) + ir[0] * ir[2], lm);
    setMacAndIr<3>((B * prevIr.z) + ir[0] * ir[3], lm);
    pushColor();
}

void GTE::ncs(int n) {
    multiplyMatrixByVector(light, v[n]);
    multiplyMatrixByVector(color, toVector(ir), backgroundColor);
    pushColor();
}

void GTE::nct() {
    ncs(0);
    ncs(1);
    ncs(2);
}

void GTE::nccs(int n) {
    multiplyMatrixByVector(light, v[n]);
    multiplyMatrixByVector(color, toVector(ir), backgroundColor);
    multiplyVectors(Vector<int16_t>(R, G, B), toVector(ir));
    pushColor();
}

void GTE::cc() {
    multiplyMatrixByVector(color, toVector(ir), backgroundColor);
    multiplyVectors(Vector<int16_t>(R, G, B), toVector(ir));
    pushColor();
}

void GTE::cdp() {
    multiplyMatrixByVector(color, toVector(ir), backgroundColor);

    auto prevIr = toVector(ir);

    setMacAndIr<1>(((int64_t)farColor.r << 12) - (R * ir[1]));
    setMacAndIr<2>(((int64_t)farColor.g << 12) - (G * ir[2]));
    setMacAndIr<3>(((int64_t)farColor.b << 12) - (B * ir[3]));

    setMacAndIr<1>((R * prevIr.x) + ir[0] * ir[1], lm);
    setMacAndIr<2>((G * prevIr.y) + ir[0] * ir[2], lm);
    setMacAndIr<3>((B * prevIr.z) + ir[0] * ir[3], lm);
    pushColor();
}

void GTE::ncdt() {
    ncds(0);
    ncds(1);
    ncds(2);
}

void GTE::ncct() {
    nccs(0);
    nccs(1);
    nccs(2);
}

void GTE::dpct() {
    dpcs(true);
    dpcs(true);
    dpcs(true);
}

void GTE::dpcs(bool useRGB0) {
    // TODO: Change to Color struct
    int16_t r = useRGB0 ? rgb[0].read(0) << 4 : R;
    int16_t g = useRGB0 ? rgb[0].read(1) << 4 : G;
    int16_t b = useRGB0 ? rgb[0].read(2) << 4 : B;

    Screenshot *screenshot = screenshot->getInstance();
    if (screenshot->disableFog) {
        setMacAndIr<1>((int64_t)(r << 12));
        setMacAndIr<2>((int64_t)(g << 12));
        setMacAndIr<3>((int64_t)(b << 12));
    } else {
        setMacAndIr<1>(((int64_t)farColor.r << 12) - (r << 12));
        setMacAndIr<2>(((int64_t)farColor.g << 12) - (g << 12));
        setMacAndIr<3>(((int64_t)farColor.b << 12) - (b << 12));
        multiplyVectors(Vector<int16_t>(ir[0]), toVector(ir), Vector<int16_t>(r, g, b));
    }
    pushColor();
}

void GTE::dcpl() {
    auto prevIr = toVector(ir);

    setMacAndIr<1>(((int64_t)farColor.r << 12) - (R * prevIr.x));
    setMacAndIr<2>(((int64_t)farColor.g << 12) - (G * prevIr.y));
    setMacAndIr<3>(((int64_t)farColor.b << 12) - (B * prevIr.z));

    setMacAndIr<1>(R * prevIr.x + ir[0] * ir[1], lm);
    setMacAndIr<2>(G * prevIr.y + ir[0] * ir[2], lm);
    setMacAndIr<3>(B * prevIr.z + ir[0] * ir[3], lm);
    pushColor();
}

void GTE::intpl() {
    auto prevIr = toVector(ir);

    setMacAndIr<1>(((int64_t)farColor.r << 12) - (prevIr.x << 12));
    setMacAndIr<2>(((int64_t)farColor.g << 12) - (prevIr.y << 12));
    setMacAndIr<3>(((int64_t)farColor.b << 12) - (prevIr.z << 12));

    multiplyVectors(Vector<int16_t>(ir[0]), toVector(ir), prevIr);
    pushColor();
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
        flag.divide_overflow = 1;
        return 0x1ffff;
    }
    uint64_t n = (((uint64_t)h * 0x20000 / (uint64_t)sz3) + 1) / 2;
    if (n > 0x1ffff) {
        flag.divide_overflow = 1;
        return 0x1ffff;
    }
    return (uint32_t)n;
}

uint32_t GTE::recip(uint16_t divisor) {
    int32_t x = 0x101 + unrTable[((divisor & 0x7fff) + 0x40) >> 7];

    int32_t tmp = (((int32_t)divisor * -x) + 0x80) >> 8;
    int32_t tmp2 = ((x * (131072 + tmp)) + 0x80) >> 8;

    return tmp2;
}

uint32_t GTE::divideUNR(uint32_t lhs, uint32_t rhs) {
    if (!(rhs * 2 > lhs)) {
        flag.divide_overflow = 1;
        return 0x1ffff;
    }

    size_t shift = countLeadingZeroes16(rhs);
    lhs <<= shift;
    rhs <<= shift;

    uint32_t reciprocal = recip(rhs | 0x8000);

    uint32_t result = ((uint64_t)lhs * reciprocal + 0x8000) >> 16;

    if (result > 0x1ffff) {
        return 0x1ffff;
    }
    return result;
}

void GTE::pushScreenXY(int32_t x, int32_t y) {
    s[0].x = s[1].x;
    s[0].y = s[1].y;
    s[1].x = s[2].x;
    s[1].y = s[2].y;

    s[2].x = clip(x, 0x3ff, -0x400, Flag::SX2_SATURATED);
    s[2].y = clip(y, 0x3ff, -0x400, Flag::SY2_SATURATED);
}

void GTE::pushScreenZ(int32_t z) {
    s[0].z = s[1].z;
    s[1].z = s[2].z;
    s[2].z = s[3].z;  // There is only s[3].z (no s[3].xy)

    s[3].z = clip(z, 0xffff, 0x0000, Flag::SZ3_OTZ_SATURATED);
}

void GTE::pushColor() { pushColor(mac[1] >> 4, mac[2] >> 4, mac[3] >> 4); }

void GTE::pushColor(uint32_t r, uint32_t g, uint32_t b) {
    rgb[0] = rgb[1];
    rgb[1] = rgb[2];

    rgb[2].write(0, clip(r, 0xff, 0x00, Flag::COLOR_R_SATURATED));
    rgb[2].write(1, clip(g, 0xff, 0x00, Flag::COLOR_G_SATURATED));
    rgb[2].write(2, clip(b, 0xff, 0x00, Flag::COLOR_B_SATURATED));
    rgb[2].write(3, rgbc.read(3));
}

/**
 * Rotate, translate and perspective transformation
 *
 * Multiplicate vector (V) with rotation matrix (R),
 * translate it (TR) and apply perspective transformation.
 */
void GTE::rtps(int n, bool setMAC0, bool fromRTPT) {
    float sx;
    float sy;
    float sw;
    int32_t ox;
    int32_t oy;
    int32_t oz;

    Screenshot *screenshot = screenshot->getInstance();

    if (screenshot->enabled && screenshot->dontTransform) {
        sx = v[n].x;
        sy = v[n].y;
        sw = v[n].z;
        ox = v[n].x;
        oy = v[n].y;
        oz = v[n].z;
    }

    int64_t mac3;
    if (screenshot->freeCamEnabled) {
        gte::Matrix postRotation;
        gte::Vector<int32_t> postTranslation;
        screenshot->processRotTrans(rotation, translation, &postRotation, &postTranslation);
        mac3 = multiplyMatrixByVectorRTP(postRotation, v[n], postTranslation);
    } else {
        mac3 = multiplyMatrixByVectorRTP(rotation, v[n], translation);
    }

    pushScreenZ((int32_t)(mac3 >> 12));
    int64_t h_s3z = divideUNR(h + screenshot->translationH, s[3].z);

    int16_t ir1 = ir[1];
    if (widescreenHack) ir1 = (int16_t)((int32_t)ir1 * 3 / 4);

    int32_t x = setMac<0>(h_s3z * ir1 + of[0]) >> 16;
    int32_t y = setMac<0>(h_s3z * ir[2] + of[1]) >> 16;
    pushScreenXY(x, y);

    // float sx = ir[1] / 1024.0f;
    // float sy = ir[2] / 1024.0f;
    // float sw = s[3].z / 65535.0f;
    // float sh = (float)h / 65535.0f;
    // float z = sh * sw;

    if (screenshot->debug || (screenshot->enabled && !screenshot->dontTransform)) {
        ox = ir[1];
        oy = ir[2];
        oz = s[3].z;
        sx = ox;
        sy = oy;
        sw = oz;
    }

    if (screenshot->debug || screenshot->enabled) {
        if (screenshot->flipX) {
            x = -x;
        }
        if (screenshot->flipY) {
            y = -y;
        }
        if (screenshot->horizontalScale != 100) {
            x *= (float)screenshot->horizontalScale / 100.0f;
        }
        if (screenshot->verticalScale != 100) {
            y *= (float)screenshot->verticalScale / 100.0f;
        }
        screenshot->addVertex(sx, sy, sw, x, y, ox, oy, oz, h);

        if (fromRTPT) {
            ScreenshotVertex *v0 = &screenshot->vertexBuffer[screenshot->vertexBuffer.size() - 1];
            ScreenshotVertex *v1 = &screenshot->vertexBuffer[screenshot->vertexBuffer.size() - 2];
            ScreenshotVertex *v2 = &screenshot->vertexBuffer[screenshot->vertexBuffer.size() - 3];
            screenshot->subGroupIndex++;
            v0->subGroup = screenshot->subGroupIndex;
            v1->subGroup = screenshot->subGroupIndex;
            v2->subGroup = screenshot->subGroupIndex;
        }
    }

    if (setMAC0) {
        int64_t mac0 = setMac<0>(h_s3z * dqa + dqb);
        ir[0] = clip(mac0 >> 12, 0x1000, 0x0000, Flag::IR0_SATURATED);
    }
}

/**
 * Same as RTPS, but repeated for vector 0, 1 and 2
 */
void GTE::rtpt() {
    rtps(0, false);
    rtps(1, false);
    rtps(2, true, true);
}

/**
 * Calculate average of 3 z values
 */
void GTE::avsz3() { setOtz(setMac<0>((int64_t)zsf3 * (s[1].z + s[2].z + s[3].z))); }

/**
 * Calculate average of 4 z values
 */
void GTE::avsz4() { setOtz(setMac<0>((int64_t)zsf4 * (s[0].z + s[1].z + s[2].z + s[3].z))); }

void GTE::mvmva(int mx, int vx, int tx) {
    Matrix Mx;
    if (mx == 0) {
        Mx = rotation;
    } else if (mx == 1) {
        Mx = light;
    } else if (mx == 2) {
        Mx = color;
    } else {
        // Buggy matrix selected
        Mx[0][0] = -R;
        Mx[0][1] = R;
        Mx[0][2] = ir[0];
        Mx[1][0] = Mx[1][1] = Mx[1][2] = rotation[0][2];
        Mx[2][0] = Mx[2][1] = Mx[2][2] = rotation[1][1];
    }

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
    if (tx == 0) {
        Tx = translation;
    } else if (tx == 1) {
        Tx = backgroundColor;
    } else if (tx == 2) {
        // Buggy FarColor vector operation
        Tx = farColor;

        Vector<int64_t> result;
        // Flag is calculated from 1st component
        setIr<1>(O(1, ((int64_t)Tx.x << 12) + Mx[0][0] * V.x) >> (sf * 12));
        setIr<2>(O(2, ((int64_t)Tx.y << 12) + Mx[1][0] * V.x) >> (sf * 12));
        setIr<3>(O(3, ((int64_t)Tx.z << 12) + Mx[2][0] * V.x) >> (sf * 12));

        // But result is 2nd and 3rd components
        result.x = O(1, O(1, Mx[0][1] * V.y) + Mx[0][2] * V.z);
        result.y = O(2, O(2, Mx[1][1] * V.y) + Mx[1][2] * V.z);
        result.z = O(3, O(3, Mx[2][1] * V.y) + Mx[2][2] * V.z);

        setMacAndIr<1>(result.x, lm);
        setMacAndIr<2>(result.y, lm);
        setMacAndIr<3>(result.z, lm);
        return;

    } else {
        Tx.x = Tx.y = Tx.z = 0;
    }

    multiplyMatrixByVector(Mx, V, Tx);
}

/**
 * General purpose interpolation
 * Multiply vector (ir[1..3]) by scalar(ir[0])
 *
 * Result is also saved as 24bit color
 */
void GTE::gpf() {
    multiplyVectors(Vector<int16_t>(ir[0]), toVector(ir));
    pushColor();
}

/**
 * Same as gpf, but add mac[i]
 * Multiply vector (ir[1..3]) by scalar(ir[0]) and add mac[1..3]
 */
void GTE::gpl() {
    setMacAndIr<1>(((int64_t)mac[1] << (sf * 12)) + ir[0] * ir[1], lm);
    setMacAndIr<2>(((int64_t)mac[2] << (sf * 12)) + ir[0] * ir[2], lm);
    setMacAndIr<3>(((int64_t)mac[3] << (sf * 12)) + ir[0] * ir[3], lm);
    pushColor();
}

/**
 * Square vector
 * lm is ignored, as result cannot be negative
 */
void GTE::sqr() { multiplyVectors(toVector(ir), toVector(ir)); }

void GTE::op() {
    setMac<1>(rotation[1][1] * ir[3] - rotation[2][2] * ir[2]);
    setMac<2>(rotation[2][2] * ir[1] - rotation[0][0] * ir[3]);
    setMac<3>(rotation[0][0] * ir[2] - rotation[1][1] * ir[1]);

    setIr<1>(mac[1], lm);
    setIr<2>(mac[2], lm);
    setIr<3>(mac[3], lm);
}