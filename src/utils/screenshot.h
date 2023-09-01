#pragma once
#include <cstring>
#include <vector>
#include <fstream>
#include <iomanip>
#include <algorithm>
#include <string>
#include <map>
#include <fmt/core.h>
#include <stb_image_write.h>
#include "device/gpu/gpu.h"
#include "device/gpu/render/texture_utils.h"
#include "utils/math.h"
#include "cpu/gte/gte.h"
#include "device/gpu/render/render.h"
#include "config.h"

#define MAXATTEMPTS 100
//#define GETTPAGE(x, y) (((y / PAGEHEIGHT) * 16) + (x / PAGECOMPRESSEDWIDTH))
#define GETTPAGE(x, y) ((x & 0xFFFF) << 16 | (y & 0xFFFF))
#define PAGEWIDTH 256
#define PAGEHEIGHT 256
#define PAGECOUNT 32
#define PAGECOMPRESSEDWIDTH 64
#define PAGEBPP 4
#define DATACOUNT (PAGEWIDTH * PAGEHEIGHT * PAGEBPP)
#define GETSTRIDE (PAGEBPP * PAGEWIDTH)
#define GETOFFSET(x, y) (x * PAGEBPP + y * GETSTRIDE)
//#define GETREGIONOFFSET(x, y) (x + y * gpu::VRAM_WIDTH)
#define ADJUSTANGLE(degrees) ((degrees % 360 + 360) % 360)

struct ScreenshotTexture {
    char data[DATACOUNT];
    int32_t tpageX;
    int32_t tpageY;
    ScreenshotTexture() {
        for (int i = 0; i < PAGECOUNT; i++) {
            for (int j = 0; j < DATACOUNT; j += PAGEBPP) {
                data[j] = 0;
                data[j + 1] = 0;
                data[j + 2] = 0;
                data[j + 3] = 0;
            }
        }
    }
    void copyFrom(ScreenshotTexture *source) { memcpy(data, source->data, DATACOUNT); }
};

// struct ScreenshotTextureRegion {
//    int minX;
//    int minY;
//    int maxX;
//    int maxY;
//    ScreenshotTextureRegion(int _minX, int _minY, int _maxX, int _maxY) {
//        minX = _minX;
//        minY = _minY;
//        maxX = _maxX;
//        maxY = _maxY;
//    }
//};

struct ScreenshotVertex {
    vec3 pos;
    int32_t depth;
    int32_t projX;
    int32_t projY;
    int32_t originalX;
    int32_t originalY;
    int32_t originalZ;
    uint32_t group;
    uint32_t subGroup;
    uint32_t index;
};

struct ScreenshotFace {
    int32_t v[3];
    int32_t vremap[3];
    uint8_t r[3];
    uint8_t g[3];
    uint8_t b[3];
    int32_t s[3];
    int32_t t[3];
    int32_t tpageX;
    int32_t tpageY;
    int32_t clutX;
    int32_t clutY;
    int32_t comp;
    ivec2 projPos[3];
    bool found;
    bool semitransparent;
    uint32_t maskX;
    uint32_t maskY;
    uint32_t offsetX;
    uint32_t offsetY;
};

struct Screenshot {
   public:
    static Screenshot *getInstance() {
        static Screenshot instance;
        return &instance;
    }

   public:
    std::vector<ScreenshotVertex> vertexBuffer;
    std::vector<ScreenshotFace> faceBuffer;
    std::vector<ScreenshotTexture> textures;
    // uint32_t textureRegions[gpu::VRAM_WIDTH * gpu::VRAM_HEIGHT];

    bool enabled = false;
    int cameraSpeed = 20;
    int depthScale = 100;
    int horizontalScale = 100;
    int verticalScale = 100;
    int captureFrames = 1;
    bool dontTransform = false;
    bool doubleSided = true;
    bool applyAspectRatio = false;
    int rotationX = 0;
    int rotationY = 0;
    int rotationZ = 0;
    int translationX = 0;
    int translationY = 0;
    int translationZ = 0;
    int translationH = 0;
    int base_rotationX = 0;
    int base_rotationY = 0;
    int base_rotationZ = 0;
    bool groupObjects = false;
    bool connectToGroups = false;
    bool connectToSubGroups = true;
    bool freeCamEnabled = false;
    bool blackIsTrans = false;
    bool disableFog = true;
    bool dontProject = false;
    bool vertexOnly = false;
    bool debug = false;
    bool flipX = false;
    bool flipY = false;

    uint32_t subGroupIndex = 0;
    uint32_t groupIndex = 0;
    // uint32_t textureRegionIndex = 0;
    uint32_t remainingFrames = 0;
    uint32_t attemptCount = 0;
    uint32_t frameIndex = 0;
    uint32_t vertexIndex = 1;

    std::string folder = avocado::screenshotPath();
    int num = 1;

    const uint32_t quadIndices[3] = {0, 1, 2};
    const uint32_t triangleIndices[3] = {2, 1, 0};
    const uint32_t *indices;

    Screenshot() { fullCleanUp(); }

    void saveBase() {
        base_rotationX = rotationX;
        base_rotationY = rotationY;
        base_rotationZ = rotationZ;
        rotationX = 0;
        rotationY = 0;
        rotationZ = 0;
    }

    void resetMatrices() {
        translationX = 0;
        translationY = 0;
        translationZ = 0;
        rotationX = 0;
        rotationY = 0;
        rotationZ = 0;
        base_rotationX = 0;
        base_rotationY = 0;
        base_rotationZ = 0;
        translationH = 0;
        base_rotationX = 0;
        base_rotationY = 0;
        base_rotationZ = 0;
    }

    gte::Matrix getInvRotation() {
        gte::Matrix baseRotation = gte::rotMatrix(
            gte::Vector<int16_t>((-base_rotationX / 360.0f) * 4096, (-base_rotationY / 360.0f) * 4096, (-base_rotationZ / 360.0f) * 4096));
        gte::Matrix preRotation = gte::rotMatrix(
            gte::Vector<int16_t>((-rotationX / 360.0f) * 4096, (-rotationY / 360.0f) * 4096, (-rotationZ / 360.0f) * 4096));
        gte::Matrix postRotation = gte::mulMatrix(baseRotation, preRotation);
        return postRotation;
    }

    gte::Matrix getRotation() {
        gte::Matrix baseRotation = gte::rotMatrix(gte::Vector<int16_t>((ADJUSTANGLE(base_rotationX) / 360.0f) * 4096,
                                                                       (ADJUSTANGLE(base_rotationY) / 360.0f) * 4096,
                                                                       (ADJUSTANGLE(base_rotationZ) / 360.0f) * 4096));
        gte::Matrix preRotation = gte::rotMatrix(gte::Vector<int16_t>(
            (ADJUSTANGLE(rotationX) / 360.0f) * 4096, (ADJUSTANGLE(rotationY) / 360.0f) * 4096, (ADJUSTANGLE(rotationZ) / 360.0f) * 4096));
        gte::Matrix postRotation = gte::mulMatrix(preRotation, baseRotation);
        return postRotation;
    }

    void processRotTrans(gte::Matrix &rotation, gte::Vector<int32_t> &translation, gte::Matrix *postRotation,
                         gte::Vector<int32_t> *postTranslation) {
        // float xScale = sqrtf(rotation[0][1] * rotation[0][1] + rotation[0][2] * rotation[0][2] + rotation[0][3] * rotation[0][3]);
        // float yScale = sqrtf(rotation[1][1] * rotation[1][1] + rotation[1][2] * rotation[1][2] + rotation[1][3] * rotation[1][3]);
        // float zScale = sqrtf(rotation[2][1] * rotation[2][1] + rotation[2][2] * rotation[2][2] + rotation[2][3] * rotation[2][3]);
        // float uScale = 1.0f;
        // sqrtf(xScale * xScale + yScale * yScale + zScale * zScale) / 4096.0f / 3.5f;
        // int32_t finalTranslationX = translationX * uScale;
        // int32_t finalTranslationY = translationY * uScale;
        // int32_t finalTranslationZ = translationZ * uScale;
        gte::Matrix customRotation = getRotation();
        *postRotation = gte::mulMatrix(customRotation, rotation);
        gte::Vector<int32_t> preTranslation
            = gte::Vector<int32_t>(translation.x - translationX, translation.y - translationY, translation.z - translationZ);
        *postTranslation = gte::applyMatrix(customRotation, preTranslation);
    }

    void processInput(int8_t x, int8_t y, int8_t z, int8_t lx, int8_t ly, int8_t lz) {
        rotationX = rotationX + ly * 2;
        rotationY = rotationY + lx * 2;
        rotationZ = rotationZ + lz * 2;
        gte::Matrix invertRotation = getInvRotation();
        gte::Vector<int32_t> v(x * cameraSpeed, y * cameraSpeed, z * cameraSpeed);
        gte::Vector<int32_t> tv = gte::applyMatrix(invertRotation, v);
        translationX += tv.x;
        translationY += tv.y;
        translationZ += tv.z;
    }

    uint32_t getTextureIndex(uint32_t tpageX, uint32_t tpageY) {
        tpageX = tpageX & 1023;
        tpageY = tpageY & 511;
        uint32_t index = 0;
        for (auto it = textures.begin(); it != textures.end(); it++) {
            if (it->tpageX == tpageX && it->tpageY == tpageY) {
                return index;
            }
            index++;
        }
        ScreenshotTexture *texture = &textures.emplace_back();
        // if (textures.size() > 1) {
        //    texture->copyFrom(&textures[textures.size() - 1]);
        //}
        texture->tpageX = tpageX;
        texture->tpageY = tpageY;
        index = textures.size() - 1;
        return index;
    }

    ivec2 calculateTexel(ivec2 tex, ScreenshotFace *face) {
        // Texture is repeated outside of 256x256 window
        tex.x %= 256u;
        tex.y %= 256u;

        // Texture masking
        //  texel = (texel AND(NOT(Mask * 8))) OR((Offset AND Mask) * 8)
        tex.x = (tex.x & ~(face->maskX * 8)) | ((face->offsetX & face->maskX) * 8);
        tex.y = (tex.y & ~(face->maskY * 8)) | ((face->offsetY & face->maskY) * 8);

        return tex;
    }

    unsigned int reverse_nibbles(unsigned int x) {
        unsigned int out = 0, i;
        for (i = 0; i < 4; ++i) {
            const unsigned int byte = (x >> 8 * i) & 0xff;
            out |= byte << (24 - 8 * i);
        }
        return out;
    }

    // void addTextureRegion(int32_t minX, int32_t minY, int32_t maxX, int32_t maxY) {
    //    textureRegionIndex++;
    //    for (int y = minY; y < maxY; y++) {
    //        for (int x = minX; x < maxX; x++) {
    //            textureRegions[GETREGIONOFFSET(x, y)] = reverse_nibbles(textureRegionIndex);
    //        }
    //    }
    //}

    // uint32_t getTextureRegion(gpu::GPU *gpu, uint32_t tpageX, uint32_t tpageY, uint32_t x, uint32_t y, int32_t comp) {
    //    uint32_t divisor;
    //    switch (comp) {
    //        case 4: divisor = 4; break;
    //        case 8: divisor = 2; break;
    //        case 16: divisor = 1; break;
    //        default: return 0;
    //    }
    //    uint32_t baseTextureRegionIndex = GETREGIONOFFSET(tpageX + (x / divisor), tpageY + y);
    //    uint32_t baseTextureRegion = textureRegions[baseTextureRegionIndex];
    //    return baseTextureRegion;
    //}

    void updateTextures(gpu::GPU *gpu) {
        for (auto it = faceBuffer.begin(); it != faceBuffer.end(); it++) {
            if (it->comp == 0) {
                continue;
            }
            ivec2 texPage(it->tpageX, it->tpageY);
            ivec2 clut(it->clutX, it->clutY);
            // ivec2 texel;
            uint32_t textureIndex = getTextureIndex(texPage.x, texPage.y);
            ScreenshotTexture *texture = &textures[textureIndex];
            // texel = calculateTexel(ivec2(it->s[0], it->t[0]), &*it);
            // uint32_t textureRegionA = getTextureRegion(gpu, texPage.x, texPage.y, texel.x, texel.y, it->comp);
            // texel = calculateTexel(ivec2(it->s[1], it->t[1]), &*it);
            // uint32_t textureRegionB = getTextureRegion(gpu, texPage.x, texPage.y, texel.x, texel.y, it->comp);
            // texel = calculateTexel(ivec2(it->s[2], it->t[2]), &*it);
            // uint32_t textureRegionC = getTextureRegion(gpu, texPage.x, texPage.y, texel.x, texel.y, it->comp);
            ivec2 texel0 = calculateTexel(ivec2(it->s[0], it->t[0]), &*it);
            ivec2 texel1 = calculateTexel(ivec2(it->s[1], it->t[1]), &*it);
            ivec2 texel2 = calculateTexel(ivec2(it->s[2], it->t[2]), &*it);
            uint32_t minX = std::min(std::min(texel0.x, texel1.x), texel2.x);
            uint32_t minY = std::min(std::min(texel0.y, texel1.y), texel2.y);
            uint32_t maxX = std::min(std::max(std::max(texel0.x, texel1.x), texel2.x) + 1u, 255u);
            uint32_t maxY = std::min(std::max(std::max(texel0.y, texel1.y), texel2.y) + 1u, 255u);
            for (int y = minY; y < maxY; y++) {
                for (int x = minX; x < maxX; x++) {
                    ivec2 texel = ivec2(x, y);
                    PSXColor color;
                    switch (it->comp) {
                        case 4:
                            loadClutCacheIfRequired<ColorDepth::BIT_4>(gpu, clut);
                            color = fetchTex<ColorDepth::BIT_4>(gpu, texel, texPage);
                            break;
                        case 8:
                            loadClutCacheIfRequired<ColorDepth::BIT_8>(gpu, clut);
                            color = fetchTex<ColorDepth::BIT_8>(gpu, texel, texPage);
                            break;
                        case 16: color = fetchTex<ColorDepth::BIT_16>(gpu, texel, texPage); break;
                        default: continue;
                    }
                    uint32_t textureAddress = GETOFFSET(x, y);
                    if (textureAddress >= DATACOUNT) {
                        continue;
                    }
                    texture->data[textureAddress + 0] = color.getR();
                    texture->data[textureAddress + 1] = color.getG();
                    texture->data[textureAddress + 2] = color.getB();
                    texture->data[textureAddress + 3] = blackIsTrans && color.getR() == 0 && color.getG() == 0 && color.getB() == 0 ? 0 : 255;
                }
            }
        }
    }

    std::vector<ScreenshotVertex *> getVertices(int16_t x, int16_t y) {
        std::vector<ScreenshotVertex *> results;
        if (!enabled) {
            return results;
        }
        int32_t index = 0;
        for (auto it = vertexBuffer.begin(); it != vertexBuffer.end(); it++) {
            if (it->projX == x && it->projY == y) {
                results.push_back(&*it);
            }
            index++;
        }
        return results;
    }

    std::vector<ScreenshotVertex *> getAverageZ(int16_t sx, int16_t sy, float *averageZ, uint32_t *averageCount) {
        std::vector<ScreenshotVertex *> vertices = getVertices(sx, sy);
        if (vertices.size() == 1) {
            *averageZ += vertices[0]->pos.z;
            averageCount++;
        }
        return vertices;
    }

    ScreenshotVertex *getClosestVertex(std::vector<ScreenshotVertex *> vertices, float averageZ, uint32_t subGroup) {
        ScreenshotVertex *closest = vertices[0];
        float closestDistance = std::abs(closest->pos.z - averageZ);
        for (auto it = vertices.begin(); it != vertices.end(); it++) {
            if (connectToSubGroups && (*it)->subGroup == subGroup) {
                return (*it);
            }
            float distance = std::abs((*it)->pos.z - averageZ);
            if (distance < closestDistance) {
                closestDistance = distance;
                closest = (*it);
            }
        }
        return closest;
    }

    bool containsFace(uint32_t v0, uint32_t v1, uint32_t v2) {
        for (auto it = faceBuffer.begin(); it != faceBuffer.end(); it++) {
            if (it->v[0] == v0 && it->v[1] == v1 && it->v[2] == v2) {
                return true;
            }
        }
        return false;
    }

    void processFace(gpu::GPU *gpu, int32_t tpageX, int32_t tpageY, int32_t clutX, int32_t clutY, uint32_t comp, ivec2 pos[], RGB color[],
                     ivec2 uv[], bool semitransparent) {
        // if (!debug && !enabled) {
        //    return;
        //}
        ScreenshotVertex *v0 = nullptr;
        ScreenshotVertex *v1 = nullptr;
        ScreenshotVertex *v2 = nullptr;
        uint32_t averageCount = 0;
        uint32_t subGroup = 0;
        float averageZ = 0;
        std::vector<ScreenshotVertex *> vertices0 = getAverageZ(pos[0].x, pos[0].y, &averageZ, &averageCount);
        if (vertices0.size() == 1) {
            v0 = vertices0[0];
            subGroup = v0->subGroup;
        }
        std::vector<ScreenshotVertex *> vertices1 = getAverageZ(pos[1].x, pos[1].y, &averageZ, &averageCount);
        if (vertices1.size() == 1) {
            v1 = vertices1[0];
            subGroup = v1->subGroup;
        }
        std::vector<ScreenshotVertex *> vertices2 = getAverageZ(pos[2].x, pos[2].y, &averageZ, &averageCount);
        if (vertices2.size() == 1) {
            v2 = vertices2[0];
            subGroup = v2->subGroup;
        }
        averageZ /= averageCount;
        if (vertices0.size() > 1) {
            v0 = getClosestVertex(vertices0, averageZ, subGroup);
        }
        if (vertices1.size() > 1) {
            v1 = getClosestVertex(vertices1, averageZ, subGroup);
        }
        if (vertices2.size() > 1) {
            v2 = getClosestVertex(vertices2, averageZ, subGroup);
        }
        bool found = v0 != nullptr && v1 != nullptr && v2 != nullptr;
        if (v0 == nullptr) {
            if (!debug || enabled) {
                return;
            }
            ScreenshotVertex v0Flat;
            v0Flat.pos.x = pos[0].x;
            v0Flat.pos.y = pos[0].y;
            v0Flat.pos.z = 1.0f;
            v0Flat.projX = pos[0].x;
            v0Flat.projY = pos[0].y;
            v0Flat.index = vertexBuffer.size();
            vertexBuffer.push_back(v0Flat);
            v0 = &v0Flat;
        }
        if (v1 == nullptr) {
            if (!debug || enabled) {
                return;
            }
            ScreenshotVertex v1Flat;
            v1Flat.pos.x = pos[1].x;
            v1Flat.pos.y = pos[1].y;
            v1Flat.pos.z = 1.0f;
            v1Flat.projX = pos[1].x;
            v1Flat.projY = pos[1].y;
            v1Flat.index = vertexBuffer.size();
            vertexBuffer.push_back(v1Flat);
            v1 = &v1Flat;
        }
        if (v2 == nullptr) {
            if (!debug || enabled) {
                return;
            }
            ScreenshotVertex v2Flat;
            v2Flat.pos.x = pos[2].x;
            v2Flat.pos.y = pos[2].y;
            v2Flat.pos.z = 1.0f;
            v2Flat.projX = pos[2].x;
            v2Flat.projY = pos[2].y;
            v2Flat.index = vertexBuffer.size();
            vertexBuffer.push_back(v2Flat);
            v2 = &v2Flat;
        }
        if (containsFace(v0->index, v1->index, v2->index)) {
            return;
        }
        if (connectToGroups && !(v0->group == v1->group && v1->group == v2->group)) {
            return;
        }

        ScreenshotFace face;
        face.tpageX = tpageX;
        face.tpageY = tpageY;
        face.clutX = clutX;
        face.clutY = clutY;
        face.comp = comp;

        // auto ab = v1->pos - v0->pos;
        // auto ac = v2->pos - v0->pos;
        // auto cross = ab.x * ac.y - ab.y * ac.x;
        // auto cross = dot(getNormal(v0->pos, v1->pos, v2->pos), vec3(0.0f, 0.0f, 1.0f));
        // auto indices = tccwindices;  // cross >= 0 ? tccwindices : tcwindices;

        // RGB red;
        // red.r = 255;
        // red.g = 0;
        // red.b = 0;
        //
        // RGB blue;
        // blue.r = 0;
        // blue.g = 0;
        // blue.b = 255;
        //
        // RGB black;
        // black.r = 0;
        // black.g = 0;
        // black.b = 0;
        //
        // RGB white;
        // white.r = 255;
        // white.g = 255;
        // white.b = 255;
        //
        // RGB tcolor = isQuad ? (cross >= 0 ? red : blue) : (cross >= 0 ? black : white);

        face.found = found;

        face.projPos[0] = pos[0];
        face.projPos[1] = pos[1];
        face.projPos[2] = pos[2];

        face.v[indices[0]] = v0->index;
        face.v[indices[1]] = v1->index;
        face.v[indices[2]] = v2->index;

        face.r[indices[0]] = color[0].r;
        face.g[indices[0]] = color[0].g;
        face.b[indices[0]] = color[0].b;

        face.r[indices[1]] = color[1].r;
        face.g[indices[1]] = color[1].g;
        face.b[indices[1]] = color[1].b;

        face.r[indices[2]] = color[2].r;
        face.g[indices[2]] = color[2].g;
        face.b[indices[2]] = color[2].b;

        face.s[indices[0]] = uv[0].x;
        face.s[indices[1]] = uv[1].x;
        face.s[indices[2]] = uv[2].x;

        face.t[indices[0]] = uv[0].y;
        face.t[indices[1]] = uv[1].y;
        face.t[indices[2]] = uv[2].y;

        face.semitransparent = semitransparent;

        face.maskX = gpu->gp0_e2.maskX;
        face.maskY = gpu->gp0_e2.maskY;
        face.offsetX = gpu->gp0_e2.offsetX;
        face.offsetY = gpu->gp0_e2.offsetY;
        // face.gp0_e6 = gpu->gp0_e6;

        faceBuffer.push_back(face);
    }

    void addVertex(float x, float y, float z, int32_t projX, int32_t projY, int32_t originalX, int32_t originalY, int32_t originalZ,
                   int32_t depth) {
        if (!enabled && !debug) {
            return;
        }
        for (auto it = vertexBuffer.begin(); it != vertexBuffer.end(); it++) {
            if (it->projX == projX && it->projY == projY && it->pos.z == z) {
                return;
            }
        }
        ScreenshotVertex vertex;
        vertex.pos.x = x;
        vertex.pos.y = y;
        vertex.pos.z = z;
        vertex.originalX = originalX;
        vertex.originalY = originalY;
        vertex.originalZ = originalZ;
        vertex.projX = projX;
        vertex.projY = projY;
        vertex.depth = depth;
        vertex.group = groupIndex;
        vertex.index = vertexBuffer.size();
        vertexBuffer.push_back(vertex);
    }

    uint32_t outputVertex(std::ofstream &stream, float x, float y, float z, float r, float g, float b, float u,
                          float v, vec3 normal,
                          uint32_t comp) {
        float multiplicator = comp == 0 ? 1.0f : 2.0f;
        stream << "vt " << std::setprecision(6) << u << " " << std::setprecision(6) << 1.0f - v << "\n";
        stream << "vn " << std::setprecision(6) << normal.x << " " << std::setprecision(6) << normal.y << " " << std::setprecision(6)
               << normal.z << "\n";
        stream << "v " << std::setprecision(6) << x << " " << std::setprecision(6) << y << " " << std::setprecision(6) << z << " "
               << std::setprecision(6) << std::min(r * multiplicator, 1.0f) << " " << std::setprecision(6)
               << std::min(g * multiplicator, 1.0f) << " " << std::setprecision(6) << std::min(b * multiplicator, 1.0f) << "\n";
        return vertexIndex++;
    }

    void cleanUp() {
        vertexBuffer.clear();
        faceBuffer.clear();
        textures.clear();
        groupIndex = 0;
        subGroupIndex = 0;
        attemptCount = 0;
        vertexIndex = 1;
    }

    void fullCleanUp() {
        // memset(textureRegions, 0, gpu::VRAM_HEIGHT * gpu::VRAM_WIDTH * 4);
        cleanUp();
    }

    void groupFace(std::map<uint32_t, std::vector<ScreenshotFace>> &groups, ScreenshotVertex &screenshotVertex,
                   ScreenshotFace &screenshotFace) {
        uint32_t groupNumber = groupObjects ? screenshotVertex.group : 0;
        auto group = groups.find(groupNumber);
        if (group == groups.end()) {
            groups.emplace(groupNumber, std::vector<ScreenshotFace>{screenshotFace});
        } else {
            group->second.push_back(screenshotFace);
        }
    }

    vec3 getNormal(vec3 a, vec3 b, vec3 c) {
        vec3 side1 = b - a;
        vec3 side2 = c - a;
        return vec3::normalize(vec3::cross(side1, side2));
    }

    void debugDraw(gpu::GPU *gpu) {
        if (!debug) {
            return;
        }
        PSXColor black(0, 0, 0);
        PSXColor red(255, 0, 0);
        PSXColor green(0, 255, 0);
        // PSXColor blue(0, 0, 255);
        if (debug) {
            for (auto it = vertexBuffer.begin(); it != vertexBuffer.end(); it++) {
                Render::drawDebugCross(gpu, red, it->projX, it->projY);
            }
            std::vector<primitive::Line> lines;
            for (auto it = faceBuffer.begin(); it != faceBuffer.end(); it++) {
                ScreenshotVertex *v0 = &vertexBuffer[it->v[0]];
                ScreenshotVertex *v1 = &vertexBuffer[it->v[1]];
                ScreenshotVertex *v2 = &vertexBuffer[it->v[2]];
                primitive::Line line0;
                line0.pos[0] = ivec2(v0->projX, v0->projY);
                line0.pos[1] = ivec2(v1->projX, v1->projY);
                primitive::Line line1;
                line1.pos[0] = ivec2(v1->projX, v1->projY);
                line1.pos[1] = ivec2(v2->projX, v2->projY);
                primitive::Line line2;
                line2.pos[0] = ivec2(v2->projX, v2->projY);
                line2.pos[1] = ivec2(v0->projX, v0->projY);
                lines.push_back(line0);
                lines.push_back(line1);
                lines.push_back(line2);
            }
            Render::drawDebugLines(gpu, black, lines);
            for (auto it = faceBuffer.begin(); it != faceBuffer.end(); it++) {
                ScreenshotVertex *v0 = &vertexBuffer[it->v[0]];
                ScreenshotVertex *v1 = &vertexBuffer[it->v[1]];
                ScreenshotVertex *v2 = &vertexBuffer[it->v[2]];
                Render::drawDebugCross(gpu, green, v0->projX, v0->projY);
                Render::drawDebugCross(gpu, green, v1->projX, v1->projY);
                Render::drawDebugCross(gpu, green, v2->projX, v2->projY);
            }
        }
    }

    void flushBuffer(gpu::GPU *gpu) {
        if (enabled) {
            if (!vertexOnly && faceBuffer.size() > 0 || vertexOnly && vertexBuffer.size() > 0) {
                std::ofstream stream(fmt::format("{}/output_{}.obj", folder, num));
                stream << fmt::format("mtllib output_{}.mtl\n", num);

                float yScale
                    = applyAspectRatio ? gpu->gp1_08.getHorizontalResoulution() / (float)gpu->gp1_08.getVerticalResoulution() : 1.0f;
                float zScale = (depthScale / 100.0f) * yScale;

                if (!vertexOnly) {
                    for (auto it = faceBuffer.begin(); it != faceBuffer.end(); it++) {
                        ScreenshotVertex v0 = vertexBuffer[it->v[0]];
                        ScreenshotVertex v1 = vertexBuffer[it->v[1]];
                        ScreenshotVertex v2 = vertexBuffer[it->v[2]];
                        vec3 normal = getNormal(v0.pos, v1.pos, v2.pos);
                        it->vremap[0] = outputVertex(stream, v0.pos.x, -v0.pos.y * yScale, -v0.pos.z * zScale, it->r[0] / 255.0f,
                                                     it->g[0] / 255.0f, it->b[0] / 255.0f, it->s[0] / 255.0f, it->t[0] / 255.0f,
                                                     normal, it->comp);
                        it->vremap[1] = outputVertex(stream, v1.pos.x, -v1.pos.y * yScale, -v1.pos.z * zScale, it->r[1] / 255.0f,
                                                     it->g[1] / 255.0f, it->b[1] / 255.0f, it->s[1] / 255.0f, it->t[1] / 255.0f,
                                                     normal, it->comp);
                        it->vremap[2] = outputVertex(stream, v2.pos.x, -v2.pos.y * yScale, -v2.pos.z * zScale, it->r[2] / 255.0f,
                                                     it->g[2] / 255.0f, it->b[2] / 255.0f, it->s[2] / 255.0f, it->t[2] / 255.0f,
                                                     normal, it->comp);
                    }
                    std::map<uint32_t, std::vector<ScreenshotFace>> groups;
                    for (auto it : faceBuffer) {
                        ScreenshotVertex v0 = vertexBuffer[it.v[0]];
                        ScreenshotVertex v1 = vertexBuffer[it.v[1]];
                        ScreenshotVertex v2 = vertexBuffer[it.v[2]];
                        groupFace(groups, v0, it);
                        groupFace(groups, v1, it);
                        groupFace(groups, v2, it);
                    }
                    for (auto it = groups.begin(); it != groups.end(); it++) {
                        stream << "g geometry_" << it->first << "\n";
                        for (ScreenshotFace screenshotFace : it->second) {
                            uint32_t vremap0 = screenshotFace.vremap[0];
                            uint32_t vremap1 = screenshotFace.vremap[1];
                            uint32_t vremap2 = screenshotFace.vremap[2];
                            uint32_t textureIndex = getTextureIndex(screenshotFace.tpageX, screenshotFace.tpageY);
                            // uint32_t pageId = GETTPAGE(screenshotFace->tpageX, screenshotFace->tpageY);
                            stream << fmt::format("usemtl output_{}_tex{}\n", num, textureIndex);
                            stream << "f " << vremap0 << "/" << vremap0 << "/" << vremap0 << " " << vremap1 << "/" << vremap1 << "/"
                                   << vremap1 << " " << vremap2 << "/" << vremap2 << "/" << vremap2 << "\n";
                        }
                    }
                } else {
                    for (auto it = vertexBuffer.begin(); it != vertexBuffer.end(); it++) {
                        outputVertex(stream, it->pos.x, -it->pos.y * yScale, -it->pos.z * zScale, 0, 0, 0, 0, 0, vec3(0, 0, 0), 0);
                    }
                }
                std::ofstream streamMtl(fmt::format("{}/output_{}.mtl", folder, num));
                // for (uint32_t i = 0; i < PAGECOUNT; i++) {
                for (auto it = textures.begin(); it != textures.end(); it++) {
                    uint32_t textureIndex = getTextureIndex(it->tpageX, it->tpageY);
                    // uint32_t pageId = GETTPAGE(it->tpageX, it->tpageY);
                    ScreenshotTexture *texture = &textures[textureIndex];

                    auto texFile = fmt::format("output_{}_tex{}.png", num, textureIndex);
                    stbi_write_png(fmt::format("{}/{}",folder, texFile).c_str(), PAGEWIDTH, PAGEHEIGHT, PAGEBPP, texture->data, GETSTRIDE);
                    streamMtl << fmt::format("newmtl output_{}_tex{}\n", num, textureIndex);
                    streamMtl << "Ka 0.000000 0.000000 0.000000"
                              << "\n";
                    streamMtl << "Kd 1.000000 1.000000 1.000000"
                              << "\n";
                    streamMtl << "Ks 0.000000 0.000000 0.000000"
                              << "\n";
                    streamMtl << "map_Kd " << texFile << "\n";
                }

                streamMtl.close();
                stream.close();
                toast(fmt::format("3d screenshot saved to output_{}.obj", num));
                
                num++;
                frameIndex++;
                
                cleanUp();
            }
            attemptCount++;
        }
        if (debug) {
            debugDraw(gpu);
        }
        if (debug || (frameIndex >= captureFrames || attemptCount > MAXATTEMPTS)) {
            cleanUp();
            frameIndex = 0;
            enabled = false;
        }
    }

    // template <class Archive>
    // void serialize(Archive &ar) {
    //    ar(textureRegions);
    //    ar(textureRegionIndex);
    //}
};