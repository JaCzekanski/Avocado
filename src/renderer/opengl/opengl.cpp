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

    createRenderBuffer();
    createBlitBuffer();
    createRenderTexture();
    createVramTexture();

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

void OpenGL::createRenderBuffer() {
    glGenVertexArrays(1, &renderVao);
    glBindVertexArray(renderVao);

    glGenBuffers(1, &renderVbo);
    glBindBuffer(GL_ARRAY_BUFFER, renderVbo);
    glBufferData(GL_ARRAY_BUFFER, bufferSize * sizeof(gpu::Vertex), NULL, GL_DYNAMIC_DRAW);

    renderShader->getAttrib("position").pointer(2, GL_INT, sizeof(gpu::Vertex), 0);
    renderShader->getAttrib("color").pointer(3, GL_INT, sizeof(gpu::Vertex), 2 * sizeof(int));
    renderShader->getAttrib("texcoord").pointer(2, GL_INT, sizeof(gpu::Vertex), 5 * sizeof(int));
    renderShader->getAttrib("bitcount").pointer(1, GL_INT, sizeof(gpu::Vertex), 7 * sizeof(int));
    renderShader->getAttrib("clut").pointer(2, GL_INT, sizeof(gpu::Vertex), 8 * sizeof(int));
    renderShader->getAttrib("texpage").pointer(2, GL_INT, sizeof(gpu::Vertex), 10 * sizeof(int));
    renderShader->getAttrib("flags").pointer(1, GL_INT, sizeof(gpu::Vertex), 12 * sizeof(int));

    glBindVertexArray(0);
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

    // Does effectively nothing since I read texels, not sample it
    bool smoothing = config["options"]["graphics"]["filtering"];
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, smoothing ? GL_LINEAR : GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, smoothing ? GL_LINEAR : GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glBindTexture(GL_TEXTURE_2D, 0);
}

void OpenGL::createRenderTexture() {
    renderWidth = 640 * 2;
    renderHeight = 480 * 2;

    glGenTextures(1, &renderTex);
    glBindTexture(GL_TEXTURE_2D, renderTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, renderWidth, renderHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glBindTexture(GL_TEXTURE_2D, 0);

    // TODO: Make C++ wrappers like shaders
    glGenFramebuffers(1, &renderFb);
    glBindFramebuffer(GL_FRAMEBUFFER, renderFb);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, renderTex, 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        printf("ERROR::FRAMEBUFFER:: Framebuffer is not complete!\n");
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void OpenGL::updateTextureParameters() {
    // bool smoothing = config["options"]["graphics"]["filtering"];

    // glBindTexture(GL_TEXTURE_2D, renderTex);
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, smoothing ? GL_LINEAR : GL_NEAREST);
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, smoothing ? GL_LINEAR : GL_NEAREST);
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
}

void OpenGL::renderVertices(gpu::GPU* gpu) {
    static glm::vec2 lastPos;
    auto& buffer = gpu->vertices;
    if (buffer.empty()) return;

    renderShader->use();

    glBindVertexArray(renderVao);
    glBindBuffer(GL_ARRAY_BUFFER, renderVbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(gpu::Vertex) * buffer.size(), buffer.data());

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, vramTex);

    glUniform2f(renderShader->getUniform("displayAreaPos"), lastPos.x, lastPos.y);
    glUniform2f(renderShader->getUniform("displayAreaSize"), gpu->gp1_08.getHorizontalResoulution(), gpu->gp1_08.getVerticalResoulution());

    glViewport(0, 0, renderWidth, renderHeight);
    glBindFramebuffer(GL_FRAMEBUFFER, renderFb);
    glDrawArrays(GL_TRIANGLES, 0, buffer.size());

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    lastPos = glm::vec2(gpu->displayAreaStartX, gpu->displayAreaStartY);
    buffer.clear();
}

void OpenGL::renderSecondStage() {
    blitShader->use();
    glBindVertexArray(blitVao);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, renderTex);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void OpenGL::render(gpu::GPU* gpu) {
    // Viewport settings
    aspect = config["options"]["graphics"]["widescreen"] ? RATIO_16_9 : RATIO_4_3;
    bool smoothing = config["options"]["graphics"]["filtering"];

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
        // bb = makeBlitBuf(0,0,1024,512);

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

    // Clear framebuffer
    glClearColor(0.f, 0.f, 0.f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT);

    // Copy VRAM to GPU
    glBindTexture(GL_TEXTURE_2D, vramTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, gpu::VRAM_WIDTH, gpu::VRAM_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_SHORT_1_5_5_5_REV, gpu->vram.data());
    // TODO: Remove 1_5_5_5, move to RGB888

    // Render all GPU commands
    renderVertices(gpu);

    // TODO: Not working
    // Blit rendered screen
    // For OpenGL < 3.2
    // glViewport(x, y, w, h);
    // renderSecondStage();

    // For OpenGL > 3.2
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, renderFb);
    glDrawBuffer(GL_BACK);
    glBlitFramebuffer(0, 0, renderWidth, renderHeight, x, y, w, h, GL_COLOR_BUFFER_BIT, smoothing ? GL_LINEAR : GL_NEAREST);

    glViewport(0, 0, width, height);
}
