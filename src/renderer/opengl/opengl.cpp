#include "opengl.h"
#include <SDL.h>
#include <algorithm>
#include "config.h"
#include "device/gpu/gpu.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>
#include "device/gpu/render/texture_utils.h"
#include "utils/string.h"

using std::make_unique;

bool OpenGL::init() {
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, VERSION_MAJOR);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, VERSION_MINOR);
    // SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
    // SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 2);
    SDL_GL_SetSwapInterval(1);

    configObserver.registerCallback(Event::Graphics, [&]() { setup(); });

    return true;
}

bool OpenGL::loadExtensions() { return gladLoadGLLoader(SDL_GL_GetProcAddress) != 0; }

bool OpenGL::loadShaders() {
    renderShader = std::make_unique<Program>("data/shader/render");
    if (!renderShader->load()) {
        printf("[GL] Cannot load render shader: %s\n", renderShader->getError().c_str());
        return false;
    }

    blitShader = std::make_unique<Program>("data/shader/blit");
    if (!blitShader->load()) {
        printf("[GL] Cannot load blit shader: %s\n", blitShader->getError().c_str());
        return false;
    }
    return true;
}

bool OpenGL::setup() {
    if (!loadExtensions()) {
        printf("[GL] Cannot initialize glad\n");
        return false;
    }

    // Check actual version of OpenGL context
    if (GLVersion.major * 10 + GLVersion.minor < VERSION_MAJOR * 10 + VERSION_MINOR) {
        printf("Unable to initialize OpenGL context for version %d.%d (got %d.%d)\n", VERSION_MAJOR, VERSION_MINOR, GLVersion.major,
               GLVersion.minor);
        return false;
    }

    if (!loadShaders()) return false;

    // OpenGL 3.2 requires VAO to be used
    // I'm binding single one for whole program - it isn't optimal
    // but will make porting to GLES2/WebGL1 much easier
    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    bool filtering = config["options"]["graphics"]["filtering"];

    renderWidth = config["options"]["graphics"]["resolution"]["width"];
    renderHeight = config["options"]["graphics"]["resolution"]["height"];

    renderBuffer = std::make_unique<Buffer>(bufferSize * sizeof(gpu::Vertex));
    renderTex = std::make_unique<Texture>(renderWidth, renderHeight, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE, filtering);
    renderFramebuffer = std::make_unique<Framebuffer>(renderTex->get());

    blitBuffer = std::make_unique<Buffer>(makeBlitBuf().size() * sizeof(BlitStruct));
    vramTex = std::make_unique<Texture>(1024, 512, GL_RGBA, GL_RGBA, GL_FLOAT, false);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

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

void OpenGL::bindRenderAttributes() {
    renderShader->getAttrib("position").pointer(2, GL_INT, sizeof(gpu::Vertex), 1 * sizeof(int));
    renderShader->getAttrib("color").pointer(3, GL_INT, sizeof(gpu::Vertex), 3 * sizeof(int));
    renderShader->getAttrib("texcoord").pointer(2, GL_INT, sizeof(gpu::Vertex), 6 * sizeof(int));
    renderShader->getAttrib("bitcount").pointer(1, GL_INT, sizeof(gpu::Vertex), 8 * sizeof(int));
    renderShader->getAttrib("clut").pointer(2, GL_INT, sizeof(gpu::Vertex), 9 * sizeof(int));
    renderShader->getAttrib("texpage").pointer(2, GL_INT, sizeof(gpu::Vertex), 11 * sizeof(int));
    renderShader->getAttrib("flags").pointer(1, GL_INT, sizeof(gpu::Vertex), 13 * sizeof(int));
    renderShader->getAttrib("textureWindow").pointer(1, GL_INT, sizeof(gpu::Vertex), 14 * sizeof(int));
}

void OpenGL::bindBlitAttributes() {
    blitShader->getAttrib("position").pointer(2, GL_FLOAT, sizeof(BlitStruct), 0);
    blitShader->getAttrib("texcoord").pointer(2, GL_FLOAT, sizeof(BlitStruct), 2 * sizeof(float));
}

void OpenGL::renderVertices(gpu::GPU* gpu) {
    static glm::vec2 lastPos;
    auto& buffer = gpu->vertices;
    if (buffer.empty()) return;

    // Unpack VRAM to native GPU format (4x float)
    std::vector<float> vramUnpacked(gpu::VRAM_HEIGHT * gpu::VRAM_WIDTH * 4);
    for (int y = 0; y < gpu::VRAM_HEIGHT; y++) {
        for (int x = 0; x < gpu::VRAM_WIDTH; x++) {
            PSXColor c = gpu->vram[y * gpu::VRAM_WIDTH + x];

            vramUnpacked[(y * gpu::VRAM_WIDTH + x) * 4 + 0] = c.r / 31.f;
            vramUnpacked[(y * gpu::VRAM_WIDTH + x) * 4 + 1] = c.g / 31.f;
            vramUnpacked[(y * gpu::VRAM_WIDTH + x) * 4 + 2] = c.b / 31.f;
            vramUnpacked[(y * gpu::VRAM_WIDTH + x) * 4 + 3] = (float)c.k;
        }
    }
    vramTex->update(vramUnpacked.data());

    renderShader->use();
    renderBuffer->bind();
    renderBuffer->update(sizeof(gpu::Vertex) * buffer.size(), buffer.data());
    bindRenderAttributes();

    // Set uniforms
    vramTex->bind(0);
    renderShader->getUniform("vram").i(0);
    renderShader->getUniform("displayAreaPos").f(lastPos.x, lastPos.y);
    renderShader->getUniform("displayAreaSize").f(gpu->gp1_08.getHorizontalResoulution(), gpu->gp1_08.getVerticalResoulution());

    renderFramebuffer->bind();
    glViewport(0, 0, renderWidth, renderHeight);

    // Render
    auto mapType = [](int type) {
        if (type == gpu::Vertex::Type::Line)
            return GL_LINES;
        else
            return GL_TRIANGLES;
    };
    int lastType = gpu::Vertex::Type::Polygon;
    int begin = 0;
    int count = 0;
    for (int i = 0; i < (int)buffer.size(); i++) {
        ++count;
        int type = buffer[i].type;

        if (i == 0 || (lastType == type && i < buffer.size() - 1)) continue;

        if (lastType != buffer[i].type) count--;
        glDrawArrays(mapType(lastType), begin, count);

        lastType = type;
        begin = begin + count;
        count = 1;
    }

    lastPos = glm::vec2(gpu->displayAreaStartX, gpu->displayAreaStartY);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void OpenGL::renderBlit(gpu::GPU* gpu) {
    blitShader->use();

    // Viewport settings
    aspect = config["options"]["graphics"]["widescreen"] ? RATIO_16_9 : RATIO_4_3;

    int x = 0;
    int y = 0;
    int w = width;
    int h = height;

    std::vector<BlitStruct> bb = makeBlitBuf(0, 0, 1024, 512);
    // bb = makeBlitBuf(gpu->displayAreaStartX, gpu->displayAreaStartY, gpu->gp1_08.getHorizontalResoulution(),
    //                  gpu->gp1_08.getVerticalResoulution());

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
    blitShader->getUniform("displayEnd").f(gpu->displayRangeX1, gpu->displayAreaStartY + y2);

    glViewport(x, y, w, h);
    blitBuffer->update(bb.size() * sizeof(BlitStruct), bb.data());

    blitBuffer->bind();
    bindBlitAttributes();

    renderTex->bind(0);
    blitShader->getUniform("renderBuffer").i(0);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void OpenGL::render(gpu::GPU* gpu) {
    // Clear framebuffer
    glClearColor(0.f, 0.f, 0.f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT);

    // Render all GPU commands
    renderVertices(gpu);

    // Blit rendered polygons to screen
    // For OpenGL < 3.2
    renderBlit(gpu);

    // For OpenGL > 3.2
    // glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    // glBindFramebuffer(GL_READ_FRAMEBUFFER, renderFramebuffer->get());
    // glDrawBuffer(GL_BACK);
    // glBlitFramebuffer(0, 0, renderWidth, renderHeight, x, y, w, h, GL_COLOR_BUFFER_BIT, smoothing ? GL_LINEAR : GL_NEAREST);

    Buffer::currentId = 0;
    Framebuffer::currentId = 0;
    // glViewport(0, 0, width, height);
}
