#include "render.h"
#include <algorithm>
#include "device/gpu/psx_color.h"
#include "dither.h"
#include "texture_utils.h"
#include "utils/macros.h"

#undef VRAM
#define VRAM ((uint16_t(*)[gpu::VRAM_WIDTH])gpu->vram.data())

int orient2d(const ivec2& a, const ivec2& b, const ivec2& c) {  //
    return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
}

static bool isTopLeft(const ivec2 e) { return e.y < 0 || (e.y == 0 && e.x < 0); }

void calculateFillRuleBias(int bias[3], const ivec2 pos[3]) {
    // Delta constants
    const ivec2 D01(pos[1].x - pos[0].x, pos[0].y - pos[1].y);
    const ivec2 D12(pos[2].x - pos[1].x, pos[1].y - pos[2].y);
    const ivec2 D20(pos[0].x - pos[2].x, pos[2].y - pos[0].y);

    // Fill rule
    bias[0] = isTopLeft(D12) ? -1 : 0;
    bias[1] = isTopLeft(D20) ? -1 : 0;
    bias[2] = isTopLeft(D01) ? -1 : 0;
}

// TODO: Fixed point has some rounding issue, need more investigation
// #define USE_FIXED_POINT

#ifdef USE_FIXED_POINT
#define FP_PRECISION 12
#define FP_PADDING 12

using delta_t = int32_t;
#define TO_FP(x) ((x) * (1 << FP_PRECISION))
#define FROM_FP(x) ((x) >> (FP_PRECISION + FP_PADDING))
#define ADD_PADDING(x) ((delta_t)(x) << FP_PADDING)
#else

using delta_t = float;
#define TO_FP(x) (x)
#define FROM_FP(x) (x)
#define ADD_PADDING(x) (x)
#endif

struct Attributes {
    delta_t r, g, b;
    delta_t u, v;
};

struct AttributeDeltas {
    struct Delta {
        delta_t x, y;
    };

    Delta r, g, b;
    Delta u, v;
};

/**
 * p - vertex position
 * a - attribute values per vertex
 */
delta_t calculateXDelta(const ivec2 p[3], const int a[3]) {
    return (p[1].y - p[2].y) * a[0] + (p[2].y - p[0].y) * a[1] + (p[0].y - p[1].y) * a[2];
}

delta_t calculateYDelta(const ivec2 p[3], const int a[3]) {
    return (p[2].x - p[1].x) * a[0] + (p[0].x - p[2].x) * a[1] + (p[1].x - p[0].x) * a[2];
}

AttributeDeltas::Delta calculateDelta(const int area, const ivec2 p[3], const int a[3]) {
    delta_t x = ADD_PADDING(TO_FP(calculateXDelta(p, a)) / area);
    delta_t y = ADD_PADDING(TO_FP(calculateYDelta(p, a)) / area);

    return {x, y};
}

delta_t calculateStartAttribute(const int area, const ivec2 p[3], const int bias[3], const int a[3]) {
    float A = (p[1].x * p[2].y - p[2].x * p[1].y) * a[0] - bias[0];
    float B = (p[2].x * p[0].y - p[0].x * p[2].y) * a[1] - bias[1];
    float C = (p[0].x * p[1].y - p[1].x * p[0].y) * a[2] - bias[2];

#ifdef USE_FIXED_POINT
    auto half = (1 << (FP_PRECISION - 1));
#else
    auto half = 0.5f;
#endif
    return ADD_PADDING((TO_FP(A + B + C) / static_cast<float>(area)) + half);
}

template <bool isGouraudShaded, bool isTextured>
Attributes calculateStartAttributes(const primitive::Triangle& triangle) {
    ivec2 p[3] = {triangle.v[0].pos, triangle.v[1].pos, triangle.v[2].pos};

    const int area = orient2d(p[0], p[1], p[2]);
    if (area == 0) return {};

    int bias[3];
    calculateFillRuleBias(bias, p);

    Attributes attrs = {};
    if constexpr (isGouraudShaded) {
        int r[3] = {triangle.v[0].color.r, triangle.v[1].color.r, triangle.v[2].color.r};
        int g[3] = {triangle.v[0].color.g, triangle.v[1].color.g, triangle.v[2].color.g};
        int b[3] = {triangle.v[0].color.b, triangle.v[1].color.b, triangle.v[2].color.b};

        attrs.r = calculateStartAttribute(area, p, bias, r);
        attrs.g = calculateStartAttribute(area, p, bias, g);
        attrs.b = calculateStartAttribute(area, p, bias, b);
    }

    if constexpr (isTextured) {
        int u[3] = {triangle.v[0].uv.x, triangle.v[1].uv.x, triangle.v[2].uv.x};
        int v[3] = {triangle.v[0].uv.y, triangle.v[1].uv.y, triangle.v[2].uv.y};

        attrs.u = calculateStartAttribute(area, p, bias, u);
        attrs.v = calculateStartAttribute(area, p, bias, v);
    }

    return attrs;
}

template <bool isGouraudShaded, bool isTextured>
AttributeDeltas calculateDeltas(const primitive::Triangle& triangle) {
    ivec2 p[3] = {triangle.v[0].pos, triangle.v[1].pos, triangle.v[2].pos};

    const int area = orient2d(p[0], p[1], p[2]);
    if (area == 0) return {};

    AttributeDeltas deltas = {};
    if constexpr (isGouraudShaded) {
        int r[3] = {triangle.v[0].color.r, triangle.v[1].color.r, triangle.v[2].color.r};
        int g[3] = {triangle.v[0].color.g, triangle.v[1].color.g, triangle.v[2].color.g};
        int b[3] = {triangle.v[0].color.b, triangle.v[1].color.b, triangle.v[2].color.b};

        deltas.r = calculateDelta(area, p, r);
        deltas.g = calculateDelta(area, p, g);
        deltas.b = calculateDelta(area, p, b);
    }

    if constexpr (isTextured) {
        int u[3] = {triangle.v[0].uv.x, triangle.v[1].uv.x, triangle.v[2].uv.x};
        int v[3] = {triangle.v[0].uv.y, triangle.v[1].uv.y, triangle.v[2].uv.y};

        deltas.u = calculateDelta(area, p, u);
        deltas.v = calculateDelta(area, p, v);
    }

    return deltas;
}

template <bool isGouraudShaded, bool isTextured>
void addXDeltas(Attributes& attrib, AttributeDeltas& deltas, int count = 1) {
    if constexpr (isGouraudShaded) {
        attrib.r += deltas.r.x * count;
        attrib.g += deltas.g.x * count;
        attrib.b += deltas.b.x * count;
    }
    if constexpr (isTextured) {
        attrib.u += deltas.u.x * count;
        attrib.v += deltas.v.x * count;
    }
}

template <bool isGouraudShaded, bool isTextured>
void addYDeltas(Attributes& attrib, AttributeDeltas& deltas, int count = 1) {
    if constexpr (isGouraudShaded) {
        attrib.r += deltas.r.y * count;
        attrib.g += deltas.g.y * count;
        attrib.b += deltas.b.y * count;
    }
    if constexpr (isTextured) {
        attrib.u += deltas.u.y * count;
        attrib.v += deltas.v.y * count;
    }
}

RGB dither(const RGB color, const ivec2 p) {
    uint8_t r = ditherLUT[p.y & 3u][p.x & 3u][color.r];
    uint8_t g = ditherLUT[p.y & 3u][p.x & 3u][color.g];
    uint8_t b = ditherLUT[p.y & 3u][p.x & 3u][color.b];
    return RGB(r, g, b);
}

template <ColorDepth bits, bool isSemiTransparent, bool isGouraudShaded, bool isBlended, bool checkMaskBeforeDraw, bool dithering>
void rasterizeTriangle(gpu::GPU* gpu, const primitive::Triangle& triangle) {
    // Extract common GPU state
    const auto transparency = triangle.transparency;
    const bool setMaskWhileDrawing = gpu->gp0_e6.setMaskWhileDrawing;
    const auto textureWindow = gpu->gp0_e2;
    constexpr bool isTextured = bits != ColorDepth::NONE;
    constexpr bool isDithered = dithering && isBlended;

    const ivec2 pos[3] = {triangle.v[0].pos, triangle.v[1].pos, triangle.v[2].pos};
    const RGB colorFlat = triangle.v[0].color;

    const int area = orient2d(pos[0], pos[1], pos[2]);
    if (area == 0) return;

    ivec2 min(                                     //
        std::min({pos[0].x, pos[1].x, pos[2].x}),  //
        std::min({pos[0].y, pos[1].y, pos[2].y})   //
    );
    ivec2 max(                                     //
        std::max({pos[0].x, pos[1].x, pos[2].x}),  //
        std::max({pos[0].y, pos[1].y, pos[2].y})   //
    );

    // Skip rendering when distance between vertices is bigger than 1023x511
    const ivec2 size = max - min;
    if (size.x >= 1024 || size.y >= 512) return;

    min = ivec2(                  //
        gpu->minDrawingX(min.x),  //
        gpu->minDrawingY(min.y)   //
    );
    max = ivec2(                  //
        gpu->maxDrawingX(max.x),  //
        gpu->maxDrawingY(max.y)   //
    );

    // https://fgiesen.wordpress.com/2013/02/10/optimizing-the-basic-rasterizer/

    // Delta constants
    const ivec2 D01(pos[1].x - pos[0].x, pos[0].y - pos[1].y);
    const ivec2 D12(pos[2].x - pos[1].x, pos[1].y - pos[2].y);
    const ivec2 D20(pos[0].x - pos[2].x, pos[2].y - pos[0].y);

    // Fill rule
    int bias[3];
    calculateFillRuleBias(bias, pos);

    // Calculate half-space values for first pixel
    int CY[3] = {
        orient2d(pos[1], pos[2], min) + bias[0],  //
        orient2d(pos[2], pos[0], min) + bias[1],  //
        orient2d(pos[0], pos[1], min) + bias[2]   //
    };

    Attributes startAttributes = calculateStartAttributes<isGouraudShaded, isTextured>(triangle);
    AttributeDeltas deltas = calculateDeltas<isGouraudShaded, isTextured>(triangle);

    addYDeltas<isGouraudShaded, isTextured>(startAttributes, deltas, min.y);
    addXDeltas<isGouraudShaded, isTextured>(startAttributes, deltas, min.x);

    ivec2 p;
    for (p.y = min.y; p.y <= max.y; p.y++) {
        Attributes attrib = startAttributes;
        int CX[3] = {CY[0], CY[1], CY[2]};

        for (p.x = min.x; p.x <= max.x; p.x++) {
            if ((CX[0] | CX[1] | CX[2]) > 0) {
                const PSXColor bg = VRAM[p.y][p.x];
                if constexpr (checkMaskBeforeDraw) {
                    if (bg.k) goto DONE;
                }

                RGB colorInterpolated(  //
                    FROM_FP(attrib.r),  //
                    FROM_FP(attrib.g),  //
                    FROM_FP(attrib.b)   //
                );

                if constexpr (isDithered) {
                    colorInterpolated = dither(colorInterpolated, p);
                }

                PSXColor c;
                if constexpr (bits == ColorDepth::NONE) {
                    if constexpr (!isGouraudShaded) {
                        c = colorFlat;
                    } else {
                        c = colorInterpolated;
                    }
                } else {
                    const ivec2 uv(FROM_FP(attrib.u), FROM_FP(attrib.v));
                    const ivec2 texel = maskTexel(uv, textureWindow);
                    c = fetchTex<bits>(gpu, texel, triangle.texpage, triangle.clut);
                    if (c.raw == 0x0000) goto DONE;

                    if constexpr (isBlended) {
                        if constexpr (isGouraudShaded) {
                            c = c * colorInterpolated;
                        } else {
                            c = c * colorFlat;
                        }

                        if constexpr (dithering) {
                            // Handle dither for Blended-flat
                        }
                    }
                }

                if constexpr (isSemiTransparent) {
                    if (!isTextured || c.k) {
                        c = PSXColor::blend(bg, c, transparency);
                    }
                }

                c.k |= setMaskWhileDrawing;

                VRAM[p.y][p.x] = c.raw;
            }

        DONE:
            CX[0] += D12.y;
            CX[1] += D20.y;
            CX[2] += D01.y;
            addXDeltas<isGouraudShaded, isTextured>(attrib, deltas);
        }
        CY[0] += D12.x;
        CY[1] += D20.x;
        CY[2] += D01.x;
        addYDeltas<isGouraudShaded, isTextured>(startAttributes, deltas);
    }
}

// Generate all permutations of rasterizeTriangle so that compiler can provide optimized versions of the function (no ifs in loop)
using rasterizeTriangle_t = void(gpu::GPU* gpu, const primitive::Triangle& triangle);

#define E(bits, isSemiTransparent, isGouraudShaded, isBlended, checkMaskBit, dithering) \
    &rasterizeTriangle<bitsToDepth<bits>(), isSemiTransparent, isGouraudShaded, isBlended, checkMaskBit, dithering>

/* Kotlin script for lookup array generation:

    fun Iterable<Int>.wrap(f: (Int) -> String): String =
        "{" + joinToString(",", transform = f) + "}"

    fun generateTable(): String =
        setOf(0, 4, 8, 16).wrap { bits ->
        (0..1).wrap { isSemiTransparent ->
        (0..1).wrap { isGouraudShaded ->
        (0..1).wrap { isBlended ->
        (0..1).wrap { checkMaskBit ->
        (0..1).wrap { dithering ->
            "E($bits, $isSemiTransparent, $isGouraudShaded, $isBlended, $checkMaskBit, $dithering)"
        }}}}}}

    generateTable()
*/

static constexpr rasterizeTriangle_t* rasterizeTriangleDispatchTable[4][2][2][2][2][2] =  //
    {{{{{{E(0, 0, 0, 0, 0, 0), E(0, 0, 0, 0, 0, 1)}, {E(0, 0, 0, 0, 1, 0), E(0, 0, 0, 0, 1, 1)}},
        {{E(0, 0, 0, 1, 0, 0), E(0, 0, 0, 1, 0, 1)}, {E(0, 0, 0, 1, 1, 0), E(0, 0, 0, 1, 1, 1)}}},
       {{{E(0, 0, 1, 0, 0, 0), E(0, 0, 1, 0, 0, 1)}, {E(0, 0, 1, 0, 1, 0), E(0, 0, 1, 0, 1, 1)}},
        {{E(0, 0, 1, 1, 0, 0), E(0, 0, 1, 1, 0, 1)}, {E(0, 0, 1, 1, 1, 0), E(0, 0, 1, 1, 1, 1)}}}},
      {{{{E(0, 1, 0, 0, 0, 0), E(0, 1, 0, 0, 0, 1)}, {E(0, 1, 0, 0, 1, 0), E(0, 1, 0, 0, 1, 1)}},
        {{E(0, 1, 0, 1, 0, 0), E(0, 1, 0, 1, 0, 1)}, {E(0, 1, 0, 1, 1, 0), E(0, 1, 0, 1, 1, 1)}}},
       {{{E(0, 1, 1, 0, 0, 0), E(0, 1, 1, 0, 0, 1)}, {E(0, 1, 1, 0, 1, 0), E(0, 1, 1, 0, 1, 1)}},
        {{E(0, 1, 1, 1, 0, 0), E(0, 1, 1, 1, 0, 1)}, {E(0, 1, 1, 1, 1, 0), E(0, 1, 1, 1, 1, 1)}}}}},
     {{{{{E(4, 0, 0, 0, 0, 0), E(4, 0, 0, 0, 0, 1)}, {E(4, 0, 0, 0, 1, 0), E(4, 0, 0, 0, 1, 1)}},
        {{E(4, 0, 0, 1, 0, 0), E(4, 0, 0, 1, 0, 1)}, {E(4, 0, 0, 1, 1, 0), E(4, 0, 0, 1, 1, 1)}}},
       {{{E(4, 0, 1, 0, 0, 0), E(4, 0, 1, 0, 0, 1)}, {E(4, 0, 1, 0, 1, 0), E(4, 0, 1, 0, 1, 1)}},
        {{E(4, 0, 1, 1, 0, 0), E(4, 0, 1, 1, 0, 1)}, {E(4, 0, 1, 1, 1, 0), E(4, 0, 1, 1, 1, 1)}}}},
      {{{{E(4, 1, 0, 0, 0, 0), E(4, 1, 0, 0, 0, 1)}, {E(4, 1, 0, 0, 1, 0), E(4, 1, 0, 0, 1, 1)}},
        {{E(4, 1, 0, 1, 0, 0), E(4, 1, 0, 1, 0, 1)}, {E(4, 1, 0, 1, 1, 0), E(4, 1, 0, 1, 1, 1)}}},
       {{{E(4, 1, 1, 0, 0, 0), E(4, 1, 1, 0, 0, 1)}, {E(4, 1, 1, 0, 1, 0), E(4, 1, 1, 0, 1, 1)}},
        {{E(4, 1, 1, 1, 0, 0), E(4, 1, 1, 1, 0, 1)}, {E(4, 1, 1, 1, 1, 0), E(4, 1, 1, 1, 1, 1)}}}}},
     {{{{{E(8, 0, 0, 0, 0, 0), E(8, 0, 0, 0, 0, 1)}, {E(8, 0, 0, 0, 1, 0), E(8, 0, 0, 0, 1, 1)}},
        {{E(8, 0, 0, 1, 0, 0), E(8, 0, 0, 1, 0, 1)}, {E(8, 0, 0, 1, 1, 0), E(8, 0, 0, 1, 1, 1)}}},
       {{{E(8, 0, 1, 0, 0, 0), E(8, 0, 1, 0, 0, 1)}, {E(8, 0, 1, 0, 1, 0), E(8, 0, 1, 0, 1, 1)}},
        {{E(8, 0, 1, 1, 0, 0), E(8, 0, 1, 1, 0, 1)}, {E(8, 0, 1, 1, 1, 0), E(8, 0, 1, 1, 1, 1)}}}},
      {{{{E(8, 1, 0, 0, 0, 0), E(8, 1, 0, 0, 0, 1)}, {E(8, 1, 0, 0, 1, 0), E(8, 1, 0, 0, 1, 1)}},
        {{E(8, 1, 0, 1, 0, 0), E(8, 1, 0, 1, 0, 1)}, {E(8, 1, 0, 1, 1, 0), E(8, 1, 0, 1, 1, 1)}}},
       {{{E(8, 1, 1, 0, 0, 0), E(8, 1, 1, 0, 0, 1)}, {E(8, 1, 1, 0, 1, 0), E(8, 1, 1, 0, 1, 1)}},
        {{E(8, 1, 1, 1, 0, 0), E(8, 1, 1, 1, 0, 1)}, {E(8, 1, 1, 1, 1, 0), E(8, 1, 1, 1, 1, 1)}}}}},
     {{{{{E(16, 0, 0, 0, 0, 0), E(16, 0, 0, 0, 0, 1)}, {E(16, 0, 0, 0, 1, 0), E(16, 0, 0, 0, 1, 1)}},
        {{E(16, 0, 0, 1, 0, 0), E(16, 0, 0, 1, 0, 1)}, {E(16, 0, 0, 1, 1, 0), E(16, 0, 0, 1, 1, 1)}}},
       {{{E(16, 0, 1, 0, 0, 0), E(16, 0, 1, 0, 0, 1)}, {E(16, 0, 1, 0, 1, 0), E(16, 0, 1, 0, 1, 1)}},
        {{E(16, 0, 1, 1, 0, 0), E(16, 0, 1, 1, 0, 1)}, {E(16, 0, 1, 1, 1, 0), E(16, 0, 1, 1, 1, 1)}}}},
      {{{{E(16, 1, 0, 0, 0, 0), E(16, 1, 0, 0, 0, 1)}, {E(16, 1, 0, 0, 1, 0), E(16, 1, 0, 0, 1, 1)}},
        {{E(16, 1, 0, 1, 0, 0), E(16, 1, 0, 1, 0, 1)}, {E(16, 1, 0, 1, 1, 0), E(16, 1, 0, 1, 1, 1)}}},
       {{{E(16, 1, 1, 0, 0, 0), E(16, 1, 1, 0, 0, 1)}, {E(16, 1, 1, 0, 1, 0), E(16, 1, 1, 0, 1, 1)}},
        {{E(16, 1, 1, 1, 0, 0), E(16, 1, 1, 1, 0, 1)}, {E(16, 1, 1, 1, 1, 0), E(16, 1, 1, 1, 1, 1)}}}}}};
#undef E

void Render::drawTriangle(gpu::GPU* gpu, const primitive::Triangle& triangle) {
    auto bits = (int)bitsToDepth(triangle.bits);
    auto isSemiTransparent = triangle.isSemiTransparent;
    auto isGouraudShaded = triangle.gouraudShading;
    auto isBlended = !triangle.isRawTexture;
    auto checkMaskBit = gpu->gp0_e6.checkMaskBeforeDraw;
    auto dithering = gpu->gp0_e1.dither24to15;

    auto rasterize = rasterizeTriangleDispatchTable[bits][isSemiTransparent][isGouraudShaded][isBlended][checkMaskBit][dithering];

    rasterize(gpu, triangle);
}