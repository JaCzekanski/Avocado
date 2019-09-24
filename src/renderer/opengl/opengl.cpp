#include "opengl.h"
#include <SDL.h>
#include <fmt/core.h>
#include <imgui.h>
#include <algorithm>
#include <glm/gtc/type_ptr.hpp>
#include "config.h"

OpenGL::OpenGL() {
#ifdef USE_OPENGLES
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
#else
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
#endif
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, VERSION_MAJOR);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, VERSION_MINOR);
    // SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
    // SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 2);

    busToken = bus.listen<Event::Config::Graphics>([&](auto) { setup(); });
}

OpenGL::~OpenGL() { bus.unlistenAll(busToken); }

bool OpenGL::loadExtensions() {
#ifdef __glad_h_
    return gladLoadGLLoader(SDL_GL_GetProcAddress) != 0;
#else
    return true;
#endif
}

bool OpenGL::loadShaders() {
    // PS1 GPU in Shader simulation
    renderShader = std::make_unique<Program>("data/shader/render.shader");
    if (!renderShader->load()) {
        fmt::print("[GL] Cannot load render shader: {}\n", renderShader->getError());
        return false;
    }

    // Draw/Blit texture to framebuffer (with PS1 clipping and other modifiers)
    blitShader = std::make_unique<Program>("data/shader/blit.shader");
    if (!blitShader->load()) {
        fmt::print("[GL] Cannot load blit shader: {}\n", blitShader->getError());
        return false;
    }

    // Copy texture to texture
    copyShader = std::make_unique<Program>("data/shader/copy.shader");
    if (!copyShader->load()) {
        fmt::print("[GL] Cannot load copy shader: {}\n", copyShader->getError());
        return false;
    }
    return true;
}

bool OpenGL::setup() {
    if (!loadExtensions()) {
        fmt::print("[GL] Cannot initialize glad\n");
        return false;
    }

#ifdef __glad_h_
    // Check actual version of OpenGL context
    if (GLVersion.major * 10 + GLVersion.minor < VERSION_MAJOR * 10 + VERSION_MINOR) {
        fmt::print("Unable to initialize OpenGL context for version {}.{} (got {}.{})\n", VERSION_MAJOR, VERSION_MINOR, GLVersion.major,
                   GLVersion.minor);
        return false;
    }
#endif

    if (!loadShaders()) return false;

    bool vsync = config["options"]["graphics"]["vsync"];
    if (vsync) {
        if (SDL_GL_SetSwapInterval(-1) != 0) {  // Try adaptive VSync
            SDL_GL_SetSwapInterval(1);          // Normal VSync
        }
    } else {
        SDL_GL_SetSwapInterval(0);  // No VSync
    }

    // OpenGL 3.2 requires VAO to be used
    // I'm binding single one for whole program - it isn't optimal
    // but will make porting to GLES2/WebGL1 much easier

    vao = std::make_unique<VertexArrayObject>();
    vao->bind();

    auto mode = config["options"]["graphics"]["rendering_mode"].get<RenderingMode>();
    hardwareRendering = (mode & RenderingMode::hardware) != 0;

    renderWidth = config["options"]["graphics"]["resolution"]["width"];
    renderHeight = config["options"]["graphics"]["resolution"]["height"];

    renderBuffer = std::make_unique<Buffer>(bufferSize * sizeof(gpu::Vertex));
    renderTex = std::make_unique<Texture>(renderWidth, renderHeight, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE, false);
    renderFramebuffer = std::make_unique<Framebuffer>(renderTex->get());

    blitBuffer = std::make_unique<Buffer>(makeBlitBuf().size() * sizeof(BlitStruct));

    vramTex.release();

    // Try native texture
#ifdef GL_UNSIGNED_SHORT_1_5_5_5_REV
    vramTex = std::make_unique<Texture>(gpu::VRAM_WIDTH, gpu::VRAM_HEIGHT, GL_RGBA, GL_RGBA, GL_UNSIGNED_SHORT_1_5_5_5_REV, false);
#endif

    if (vramTex && vramTex->isCreated()) {
        supportNativeTexture = true;
    } else {
        // Use compability mode
        vramTex = std::make_unique<Texture>(gpu::VRAM_WIDTH, gpu::VRAM_HEIGHT, GL_RGBA, GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1, false);
        if (!vramTex || !vramTex->isCreated()) {
            fmt::print("[GL] Unable to create VRAM texture\n");
            return false;
        }
        supportNativeTexture = false;
    }

    // Texture for 24bit mode
    vram24Tex = std::make_unique<Texture>(gpu::VRAM_WIDTH, gpu::VRAM_HEIGHT, GL_RGB, GL_RGB, GL_UNSIGNED_BYTE, false);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glUseProgram(0);

    return true;
}

std::vector<OpenGL::BlitStruct> OpenGL::makeBlitBuf(int screenX, int screenY, int screenW, int screenH, bool invert) {
    /* X,Y

       0,0      1,0


       0,1      1,1
    */

    float sx = (float)screenX / 1024.f;
    float sy = (float)screenY / 512.f;

    float sw = sx + (float)screenW / 1024.f;
    float sh = sy + (float)screenH / 512.f;

    if (invert) {
        float tmp = sy;
        sy = sh;
        sh = tmp;
    }

    return {
        {{0.f, 0.f}, {sx, sy}}, {{1.f, 0.f}, {sw, sy}}, {{1.f, 1.f}, {sw, sh}},
        {{0.f, 0.f}, {sx, sy}}, {{1.f, 1.f}, {sw, sh}}, {{0.f, 1.f}, {sx, sh}},
    };
}

void OpenGL::bindRenderAttributes() {
    renderShader->getAttrib("position").pointer(2, GL_INT, sizeof(gpu::Vertex), 1 * sizeof(int));
    renderShader->getAttrib("color").pointer(3, GL_UNSIGNED_INT, sizeof(gpu::Vertex), 3 * sizeof(int));
    renderShader->getAttrib("texcoord").pointer(2, GL_INT, sizeof(gpu::Vertex), 6 * sizeof(int));
    renderShader->getAttrib("bitcount").pointer(1, GL_UNSIGNED_INT, sizeof(gpu::Vertex), 8 * sizeof(int));
    renderShader->getAttrib("clut").pointer(2, GL_INT, sizeof(gpu::Vertex), 9 * sizeof(int));
    renderShader->getAttrib("texpage").pointer(2, GL_INT, sizeof(gpu::Vertex), 11 * sizeof(int));
    renderShader->getAttrib("flags").pointer(1, GL_UNSIGNED_INT, sizeof(gpu::Vertex), 13 * sizeof(int));
    renderShader->getAttrib("textureWindow").pointer(1, GL_UNSIGNED_INT, sizeof(gpu::Vertex), 14 * sizeof(int));
}

void OpenGL::bindBlitAttributes() {
    blitShader->getAttrib("position").pointer(2, GL_FLOAT, sizeof(BlitStruct), 0);
    blitShader->getAttrib("texcoord").pointer(2, GL_FLOAT, sizeof(BlitStruct), 2 * sizeof(float));
}

void OpenGL::bindCopyAttributes() {
    copyShader->getAttrib("position").pointer(2, GL_FLOAT, sizeof(BlitStruct), 0);
    copyShader->getAttrib("texcoord").pointer(2, GL_FLOAT, sizeof(BlitStruct), 2 * sizeof(float));
}

void OpenGL::update24bitTexture(gpu::GPU* gpu) {
    size_t dataSize = gpu::VRAM_HEIGHT * gpu::VRAM_WIDTH * 3;
    if (vram24Unpacked.size() != dataSize) {
        vram24Unpacked.resize(dataSize);
    }

    // Unpack VRAM to 8 bit RGB values (two pixels at the time)
    for (int y = 0; y < gpu::VRAM_HEIGHT; y++) {
        unsigned int gpuOffset = y * gpu::VRAM_WIDTH;
        unsigned int texOffset = y * gpu::VRAM_WIDTH * 3;
        for (int x = 0; x < gpu::VRAM_WIDTH; x += 3) {
            uint16_t c1 = gpu->vram[gpuOffset + 0];
            uint16_t c2 = gpu->vram[gpuOffset + 1];
            uint16_t c3 = gpu->vram[gpuOffset + 2];

            uint8_t r0 = (c1 & 0x00ff) >> 0;
            uint8_t g0 = (c1 & 0xff00) >> 8;
            uint8_t b0 = (c2 & 0x00ff) >> 0;
            uint8_t r1 = (c2 & 0xff00) >> 8;
            uint8_t g1 = (c3 & 0x00ff) >> 0;
            uint8_t b1 = (c3 & 0xff00) >> 8;

            vram24Unpacked[texOffset + 0] = r0;
            vram24Unpacked[texOffset + 1] = g0;
            vram24Unpacked[texOffset + 2] = b0;
            vram24Unpacked[texOffset + 3] = r1;
            vram24Unpacked[texOffset + 4] = g1;
            vram24Unpacked[texOffset + 5] = b1;

            gpuOffset += 3;
            texOffset += 6;
        }
    }
    vram24Tex->update(vram24Unpacked.data());
}

void OpenGL::updateVramTexture(gpu::GPU* gpu) {
    if (supportNativeTexture) {
        vramTex->update(gpu->vram.data());
        return;
    }

    size_t dataSize = gpu::VRAM_HEIGHT * gpu::VRAM_WIDTH;
    if (vramUnpacked.size() != dataSize) {
        vramUnpacked.resize(dataSize);
    }

    // Unpack VRAM to native GPU format
    for (int y = 0; y < gpu::VRAM_HEIGHT; y++) {
        for (int x = 0; x < gpu::VRAM_WIDTH; x++) {
            unsigned int pos = y * gpu::VRAM_WIDTH + x;

            vramUnpacked[pos] = PSXColor(gpu->vram[pos]).rev();
        }
    }
    vramTex->update(vramUnpacked.data());
}

void OpenGL::renderVertices(gpu::GPU* gpu) {
    static glm::vec2 lastPos;
    auto& buffer = gpu->vertices;
    if (buffer.empty()) {
        return;
    }

    int areaX = static_cast<int>(lastPos.x);
    int areaY = static_cast<int>(lastPos.y);
    int areaW = static_cast<int>(gpu->gp1_08.getHorizontalResoulution());
    int areaH = static_cast<int>(gpu->gp1_08.getVerticalResoulution());

    // Simulate GPU in Shader (skip if no entries in renderlist)
    glViewport(0, 0, renderWidth, renderHeight);
    renderFramebuffer->bind();

    renderShader->use();
    renderBuffer->bind();
    renderBuffer->update(sizeof(gpu::Vertex) * buffer.size(), buffer.data());
    bindRenderAttributes();

    // Set uniforms
    vramTex->bind(0);
    renderShader->getUniform("vram").i(0);
    renderShader->getUniform("displayAreaPos").f(areaX, areaY);
    renderShader->getUniform("displayAreaSize").f(areaW, areaH);

    auto mapType = [](int type) {
        if (type == gpu::Vertex::Type::Line)
            return GL_LINES;
        else
            return GL_TRIANGLES;
    };

    glBlendColor(0.25f, 0.25f, 0.25f, 0.5f);

    // Unbatched render
    using Transparency = gpu::SemiTransparency;

    for (size_t i = 0; i < buffer.size();) {
        auto type = mapType(buffer[i].type);
        int count = type == GL_TRIANGLES ? 3 : 2;

        // Skip rendering when distance between vertices is bigger than 1023x511
        bool skipRender = false;
        for (int j = 0; j < count; j++) {
            auto& v0 = buffer[i + j];
            auto& v1 = buffer[i + ((j + 1) % count)];
            int x = abs(v0.position[0] - v1.position[0]);
            int y = abs(v0.position[1] - v1.position[1]);
            if (x >= 1024 || y >= 1024) {
                skipRender = true;
                break;
            }
        }

        if (skipRender) {
            i += count;
            continue;
        }

        if (buffer[i].flags & gpu::Vertex::SemiTransparency) {
            glEnable(GL_BLEND);
            auto semi = static_cast<Transparency>((buffer[i].flags >> 5) & 3);
            // TODO: Refactor and batch
            if (semi == Transparency::Bby2plusFby2) {
                glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
                glBlendFuncSeparate(GL_CONSTANT_ALPHA, GL_CONSTANT_ALPHA, GL_ONE, GL_ZERO);
            } else if (semi == Transparency::BplusF) {
                glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
                glBlendFuncSeparate(GL_ONE, GL_ONE, GL_ONE, GL_ZERO);
            } else if (semi == Transparency::BminusF) {
                glBlendEquationSeparate(GL_FUNC_REVERSE_SUBTRACT, GL_FUNC_ADD);
                glBlendFuncSeparate(GL_ONE, GL_ONE, GL_ONE, GL_ZERO);
            } else if (semi == Transparency::BplusFby4) {
                glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
                glBlendFuncSeparate(GL_CONSTANT_COLOR, GL_ONE, GL_ONE, GL_ZERO);
            }
        } else {
            glDisable(GL_BLEND);
        }

        glDrawArrays(type, i, count);

        i += count;
    }
    lastPos = glm::vec2(gpu->displayAreaStartX, gpu->displayAreaStartY);

    glBlendColor(1.f, 1.f, 1.f, 1.f);
    glDisable(GL_BLEND);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void OpenGL::renderBlit(gpu::GPU* gpu, bool software) {
    blitShader->use();

    // Viewport settings
    aspect = config["options"]["graphics"]["widescreen"] ? RATIO_16_9 : RATIO_4_3;

    int x = 0;
    int y = 0;
    int w = width;
    int h = height;

    std::vector<BlitStruct> bb = makeBlitBuf(0, 0, 1024, 512);
    if (software) {
        bb = makeBlitBuf(gpu->displayAreaStartX, gpu->displayAreaStartY, gpu->gp1_08.getHorizontalResoulution(),
                         gpu->gp1_08.getVerticalResoulution(), true);
    }

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
    // TODO: convert xy to screen space
    blitShader->getUniform("clipLeftTop").f(0, 0);
    blitShader->getUniform("clipRightBottom").f(1024, gpu->displayAreaStartY + y2);

    glViewport(x, y, w, h);
    blitBuffer->update(bb.size() * sizeof(BlitStruct), bb.data());

    blitBuffer->bind();
    bindBlitAttributes();

    // Hack
    if (gpu->gp1_08.colorDepth == gpu::GP1_08::ColorDepth::bit24) {
        vram24Tex->bind(0);
    } else if (software) {
        vramTex->bind(0);
    } else {
        renderTex->bind(0);
    }
    blitShader->getUniform("renderBuffer").i(0);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void OpenGL::render(gpu::GPU* gpu) {
    vao->bind();
    // Clear framebuffer
    glClearColor(0.f, 0.f, 0.f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT);

    if (gpu->gp1_08.colorDepth == gpu::GP1_08::ColorDepth::bit24) {
        // HACK: Force software rendering for movies (24bit mode)
        update24bitTexture(gpu);
        renderBlit(gpu, true);
    } else {
        updateVramTexture(gpu);

        if (hardwareRendering) {
            // Render all GPU commands
            renderVertices(gpu);
        }

        // Blit rendered polygons to screen
        // For OpenGL < 3.2
        renderBlit(gpu, !hardwareRendering);
    }

    // For OpenGL > 3.2
    // glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    // glBindFramebuffer(GL_READ_FRAMEBUFFER, renderFramebuffer->get());
    // glDrawBuffer(GL_BACK);
    // glBlitFramebuffer(0, 0, renderWidth, renderHeight, x, y, w, h, GL_COLOR_BUFFER_BIT, smoothing ? GL_LINEAR : GL_NEAREST);

    Buffer::currentId = 0;
    Framebuffer::currentId = 0;
    // glViewport(0, 0, width, height);
}
