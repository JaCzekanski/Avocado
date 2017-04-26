#include "opengl.h"
#include <SDL.h>
#include <glad/glad.h>
#include <memory>
#include "shader/Program.h"
#include "device/gpu.h"

using namespace device::gpu;

bool OpenGL::init() {
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);

    if (!loadExtensions()) {
        printf("Cannot initialize glad\n");
        return false;
    }
    return true;
}

bool OpenGL::loadExtensions() { return gladLoadGLLoader(SDL_GL_GetProcAddress) != 0; }

bool OpenGL::loadShaders() {
    renderShader = std::make_unique<Program>("data/shader/render");
    if (!renderShader->load()) {
        printf("Cannot load render shader: %s\n", renderShader->getError().c_str());
        return false;
    }

    blitShader = std::make_unique<Program>("data/shader/blit");
    if (!blitShader->load()) {
        printf("Cannot load blit shader: %s\n", blitShader->getError().c_str());
        return false;
    }
    return true;
}

void OpenGL::createRenderBuffer() {
    glGenVertexArrays(1, &renderVao);
    glBindVertexArray(renderVao);

    glGenBuffers(1, &renderVbo);
    glBindBuffer(GL_ARRAY_BUFFER, renderVbo);
    glBufferData(GL_ARRAY_BUFFER, bufferSize * sizeof(Vertex), nullptr, GL_DYNAMIC_DRAW);

    renderShader->getAttrib("position").pointer(2, GL_UNSIGNED_INT, sizeof(Vertex), 0);
    renderShader->getAttrib("color").pointer(3, GL_UNSIGNED_INT, sizeof(Vertex), 2 * sizeof(int));
    renderShader->getAttrib("texcoord").pointer(2, GL_UNSIGNED_INT, sizeof(Vertex), 5 * sizeof(int));
    renderShader->getAttrib("bitcount").pointer(1, GL_UNSIGNED_INT, sizeof(Vertex), 7 * sizeof(int));
    renderShader->getAttrib("clut").pointer(2, GL_UNSIGNED_INT, sizeof(Vertex), 8 * sizeof(int));
    renderShader->getAttrib("texpage").pointer(2, GL_UNSIGNED_INT, sizeof(Vertex), 10 * sizeof(int));
    renderShader->getAttrib("flags").pointer(1, GL_UNSIGNED_INT, sizeof(Vertex), 12 * sizeof(int));

    glBindVertexArray(0);
}

std::vector<OpenGL::BlitStruct> OpenGL::makeBlitBuf(int screenX, int screenY, int screenW, int screenH) {
    /* X,Y

       0,0      1,0


       0,1      1,1
    */
    float sx = (float)screenX / 1024.f;
    float sy = (float)screenY / 512.f;
    float sw = sx + (float)screenW / 1024.f;
    float sh = sy + (float)screenH / 512.f;

    return {
        {{0.f, 0.f}, {sx, sy}}, {{1.f, 0.f}, {sw, sy}}, {{1.f, 1.f}, {sw, sh}},
        {{0.f, 0.f}, {sx, sy}}, {{1.f, 1.f}, {sw, sh}}, {{0.f, 1.f}, {sx, sh}},
    };
}

void OpenGL::createBlitBuffer() {
    auto blitVertex = makeBlitBuf();

    glGenVertexArrays(1, &blitVao);
    glBindVertexArray(blitVao);

    glGenBuffers(1, &blitVbo);
    glBindBuffer(GL_ARRAY_BUFFER, blitVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(blitVertex) * sizeof(BlitStruct), blitVertex.data(), GL_STATIC_DRAW);

    blitShader->getAttrib("position").pointer(2, GL_FLOAT, sizeof(BlitStruct), 0);
    blitShader->getAttrib("texcoord").pointer(2, GL_FLOAT, sizeof(BlitStruct), 2 * sizeof(float));

    glBindVertexArray(0);
}

void OpenGL::createVramTexture() {
    glGenTextures(1, &vramTex);
    glBindTexture(GL_TEXTURE_2D, vramTex);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1024, 512, 0, GL_RGBA, GL_UNSIGNED_SHORT_1_5_5_5_REV, nullptr);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    renderShader->use();
    glUniform1i(renderShader->getUniform("vram"), 0);

    blitShader->use();
    glUniform1i(blitShader->getUniform("renderBuffer"), 0);
}

void OpenGL::createRenderTexture() {
    glGenTextures(1, &renderTex);
    glBindTexture(GL_TEXTURE_2D, renderTex);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1024, 512, 0, GL_RGBA, GL_UNSIGNED_SHORT_1_5_5_5_REV, nullptr);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, renderTex, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

bool OpenGL::setup() {
    if (!loadShaders()) return false;

    createRenderBuffer();
    createBlitBuffer();
    createVramTexture();
    createRenderTexture();

    return true;
}

void OpenGL::renderFirstStage(const std::vector<Vertex> &renderList, device::gpu::GPU *gpu) {
    // First stage - render calls to VRAM
    if (renderList.empty()) {
        return;
    }
    renderShader->use();
    glBindVertexArray(renderVao);
    glBindBuffer(GL_ARRAY_BUFFER, renderVbo);

    glUniform2i(renderShader->getUniform("drawingOffset"), gpu->drawingOffsetX, gpu->drawingOffsetY);
    glUniform2ui(renderShader->getUniform("drawingAreaTopLeft"), gpu->drawingAreaLeft, gpu->drawingAreaTop);
    glUniform2ui(renderShader->getUniform("drawingAreaBottomRight"), gpu->drawingAreaRight, gpu->drawingAreaBottom);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, vramTex);

    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glDrawArrays(GL_TRIANGLES, 0, renderList.size());
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void OpenGL::renderSecondStage() {
    blitShader->use();
    glBindVertexArray(blitVao);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, renderTex);

    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void OpenGL::render(device::gpu::GPU *gpu) {
    // Update screen position

    int screenWidth = OpenGL::resWidth;
    int screenHeight = OpenGL::resHeight;

    std::vector<BlitStruct> bb;
    if (viewFullVram) {
        screenWidth = 1024;
        screenHeight = 512;
        bb = makeBlitBuf(0, 0, screenWidth, screenHeight);
    } else {
        bb = makeBlitBuf(gpu->displayAreaStartX, gpu->displayAreaStartY, gpu->gp1_08.getHorizontalResoulution(),
                         gpu->gp1_08.getVerticalResoulution());
    }
    glBindBuffer(GL_ARRAY_BUFFER, blitVbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, bb.size() * sizeof(BlitStruct), bb.data());

    // Update VRAM texture
    glBindTexture(GL_TEXTURE_2D, vramTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1024, 512, 0, GL_RGBA, GL_UNSIGNED_SHORT_1_5_5_5_REV, gpu->VRAM);

    // Update Render texture
    glBindTexture(GL_TEXTURE_2D, renderTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1024, 512, 0, GL_RGBA, GL_UNSIGNED_SHORT_1_5_5_5_REV, gpu->VRAM);

    auto &renderList = gpu->render();

    // Update renderlist
    if (!renderList.empty()) {
        glBindBuffer(GL_ARRAY_BUFFER, renderVbo);
        glBufferSubData(GL_ARRAY_BUFFER, 0, renderList.size() * sizeof(Vertex), renderList.data());
    }

    glViewport(0, 0, 1024, 512);
    renderFirstStage(renderList, gpu);

    glViewport(0, 0, screenWidth, screenHeight);
    renderSecondStage();

    // Read back rendered texture to VRAM
    glBindTexture(GL_TEXTURE_2D, renderTex);
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_SHORT_1_5_5_5_REV, gpu->VRAM);

    renderList.clear();
}
