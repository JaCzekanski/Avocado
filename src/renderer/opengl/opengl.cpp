#include "opengl.h"
#include <SDL.h>
#include <glad/glad.h>
#include <memory>
#include "config.h"
#include "device/gpu/gpu.h"
#include "shader/Program.h"

bool OpenGL::init() {
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, VERSION_MAJOR);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, VERSION_MINOR);
    SDL_GL_SetSwapInterval(0);

    return true;
}

bool OpenGL::loadExtensions() { return gladLoadGLLoader(SDL_GL_GetProcAddress) != 0; }

bool OpenGL::loadShaders() {
    blitShader = std::make_unique<Program>("data/shader/blit");
    if (!blitShader->load()) {
        printf("Cannot load blit shader: %s\n", blitShader->getError().c_str());
        return false;
    }
    return true;
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

void OpenGL::createRenderTexture() {
    glGenTextures(1, &renderTex);
    glBindTexture(GL_TEXTURE_2D, renderTex);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1024, 512, 0, GL_RGBA, GL_UNSIGNED_SHORT_1_5_5_5_REV, nullptr);

    updateTextureParameters();
}

void OpenGL::updateTextureParameters() {
    bool smoothing = config["options"]["graphics"]["filtering"];

    glBindTexture(GL_TEXTURE_2D, renderTex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, smoothing ? GL_LINEAR : GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, smoothing ? GL_LINEAR : GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
}

bool OpenGL::setup() {
    if (!loadExtensions()) {
        printf("Cannot initialize glad\n");
        return false;
    }

    // Check actual version of OpenGL context
    if (GLVersion.major * 10 + GLVersion.minor < VERSION_MAJOR * 10 + VERSION_MINOR) {
        printf("Unable to initialize OpenGL context for version %d.%d (got %d.%d)\n", VERSION_MAJOR, VERSION_MINOR, GLVersion.major,
               GLVersion.minor);
        return false;
    }

    if (!loadShaders()) return false;

    createBlitBuffer();
    createRenderTexture();

    return true;
}

void OpenGL::renderSecondStage() {
    blitShader->use();
    glBindVertexArray(blitVao);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, renderTex);

    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void OpenGL::render(gpu::GPU *gpu) {
    // Viewport settings
    aspect = config["options"]["graphics"]["widescreen"] ? RATIO_16_9 : RATIO_4_3;

    int x = 0;
    int y = 0;
    int w = width;
    int h = height;

    std::vector<BlitStruct> bb;
    if (viewFullVram) {
        w = 1024;
        h = 512;
        bb = makeBlitBuf(0, 0, w, h);
        glViewport(0, 0, w, h);

        glUniform2f(blitShader->getUniform("displayStart"), 0.f, 0.f);
        glUniform2f(blitShader->getUniform("displayEnd"), 1024.f, 512.f);
    } else {
        bb = makeBlitBuf(gpu->displayAreaStartX, gpu->displayAreaStartY, gpu->gp1_08.getHorizontalResoulution(),
                         gpu->gp1_08.getVerticalResoulution());

        if (width > height * aspect) {
            // Fit vertical
            w = static_cast<int>(height * aspect);
            x = static_cast<int>((width - w) / 2.f);
        } else {
            // Fit horizontal
            h = static_cast<int>(width * (1.0f / aspect));
            y = static_cast<int>((height - h) / 2.f);
        }

        float y2 = static_cast<float>(gpu->displayRangeY2 - gpu->displayRangeY1);
        if (gpu->gp1_08.getVerticalResoulution() == 480) y2 *= 2;
        glUniform2f(blitShader->getUniform("displayEnd"), gpu->displayRangeX1, gpu->displayAreaStartY + y2);
    }

    glBindBuffer(GL_ARRAY_BUFFER, blitVbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, bb.size() * sizeof(BlitStruct), bb.data());

    // Update Render texture
    updateTextureParameters();
    glBindTexture(GL_TEXTURE_2D, renderTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1024, 512, 0, GL_RGBA, GL_UNSIGNED_SHORT_1_5_5_5_REV, gpu->vram.data());
    // TODO: Remove 1_5_5_5, move to RGB888

    glClearColor(0.f, 0.f, 0.f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT);

    glViewport(x, y, w, h);
    renderSecondStage();

    glViewport(0, 0, width, height);
}
