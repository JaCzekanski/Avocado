#include "opengl.h"
#include <SDL.h>
#include <fmt/core.h>
#include <algorithm>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <imgui.h>
#include <stb_image_write.h>
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
    hardwareRendering = (mode & RenderingMode::HARDWARE) != 0;

    transparency = config["options"]["graphics"]["transparency"];

    renderWidth = config["options"]["graphics"]["resolution"]["width"];
    renderHeight = config["options"]["graphics"]["resolution"]["height"];

    renderBuffer = std::make_unique<Buffer>(bufferSize * sizeof(gpu::Vertex));
    renderTex = std::make_unique<Texture>(renderWidth, renderHeight, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE, false);
    renderFramebuffer = std::make_unique<Framebuffer>(renderTex->get());

    blitBuffer = std::make_unique<Buffer>(makeBlitBuf().size() * sizeof(BlitStruct));

    // Try native texture

    vramTex.release();
#ifdef GL_UNSIGNED_SHORT_1_5_5_5_REV
    vramTex = std::make_unique<Texture>(1024, 512, GL_RGBA, GL_RGBA, GL_UNSIGNED_SHORT_1_5_5_5_REV, false);
#endif

    if (vramTex && vramTex->isCreated()) {
        supportNativeTexture = true;
    } else {
        // Use compability mode
        vramTex = std::make_unique<Texture>(1024, 512, GL_RGBA, GL_RGBA, GL_FLOAT, false);
        supportNativeTexture = false;
    }

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
    // Hack: 24 bit is degraded to 15bit native texture here
    if (supportNativeTexture) {
        size_t dataSize = gpu::VRAM_HEIGHT * gpu::VRAM_WIDTH;
        static std::vector<uint16_t> vram24Unpacked;
        if (vram24Unpacked.size() != dataSize) {
            vram24Unpacked.resize(dataSize);
        }

        // Unpack VRAM to native GPU format (4x float)
        for (int y = 0; y < gpu::VRAM_HEIGHT; y++) {
            for (int x = 0; x < gpu::VRAM_WIDTH; x++) {
                int xMasked = (x / 2) * 3;
                uint16_t col;
                if (x % 2 == 0) {
                    uint16_t c1 = gpu->vram[y * gpu::VRAM_WIDTH + xMasked + 0];
                    uint16_t c2 = gpu->vram[y * gpu::VRAM_WIDTH + xMasked + 1];

                    uint8_t r = (c1 & 0x00ff) >> 0;
                    uint8_t g = (c1 & 0xff00) >> 8;
                    uint8_t b = (c2 & 0x00ff) >> 0;

                    col = to15bit(r, g, b);

                } else {
                    uint16_t c1 = gpu->vram[y * gpu::VRAM_WIDTH + xMasked + 1];
                    uint16_t c2 = gpu->vram[y * gpu::VRAM_WIDTH + xMasked + 2];

                    uint8_t r = (c1 & 0xff00) >> 8;
                    uint8_t g = (c2 & 0x00ff) >> 0;
                    uint8_t b = (c2 & 0xff00) >> 8;

                    col = to15bit(r, g, b);
                }

                vram24Unpacked[(y * gpu::VRAM_WIDTH + x)] = col;
            }
        }
        vramTex->update(vram24Unpacked.data());
        return;
    }

    size_t dataSize = gpu::VRAM_HEIGHT * gpu::VRAM_WIDTH * 4;
    if (vramUnpacked.size() != dataSize) {
        vramUnpacked.resize(dataSize);
    }

    // Unpack VRAM to native GPU format (4x float)
    for (int y = 0; y < gpu::VRAM_HEIGHT; y++) {
        for (int x = 0; x < gpu::VRAM_WIDTH; x++) {
            int xMasked = (x / 2) * 3;
            if (x % 2 == 0) {
                uint16_t c1 = gpu->vram[y * gpu::VRAM_WIDTH + xMasked + 0];
                uint16_t c2 = gpu->vram[y * gpu::VRAM_WIDTH + xMasked + 1];

                vramUnpacked[(y * gpu::VRAM_WIDTH + x) * 4 + 0] = ((c1 & 0x00ff) >> 0) / 255.f;
                vramUnpacked[(y * gpu::VRAM_WIDTH + x) * 4 + 1] = ((c1 & 0xff00) >> 8) / 255.f;
                vramUnpacked[(y * gpu::VRAM_WIDTH + x) * 4 + 2] = ((c2 & 0x00ff) >> 0) / 255.f;
                vramUnpacked[(y * gpu::VRAM_WIDTH + x) * 4 + 3] = 1.0;
            } else {
                uint16_t c1 = gpu->vram[y * gpu::VRAM_WIDTH + xMasked + 1];
                uint16_t c2 = gpu->vram[y * gpu::VRAM_WIDTH + xMasked + 2];

                vramUnpacked[(y * gpu::VRAM_WIDTH + x) * 4 + 0] = ((c1 & 0xff00) >> 8) / 255.f;
                vramUnpacked[(y * gpu::VRAM_WIDTH + x) * 4 + 1] = ((c2 & 0x00ff) >> 0) / 255.f;
                vramUnpacked[(y * gpu::VRAM_WIDTH + x) * 4 + 2] = ((c2 & 0xff00) >> 8) / 255.f;
                vramUnpacked[(y * gpu::VRAM_WIDTH + x) * 4 + 3] = 1.0;
            }
        }
    }

    vramTex->update(vramUnpacked.data());
}

constexpr std::array<float, 32> generateFloatLUT() {
    std::array<float, 32> lut = {{0}};
    for (int i = 0; i < 32; i++) {
        lut[i] = i / 31.f;
    }
    return lut;
}

const std::array<float, 32> floatLUT = generateFloatLUT();

void OpenGL::updateVramTexture(gpu::GPU* gpu) {
    if (supportNativeTexture) {
        vramTex->update(gpu->vram.data());
        return;
    }

    size_t dataSize = gpu::VRAM_HEIGHT * gpu::VRAM_WIDTH * 4;
    if (vramUnpacked.size() != dataSize) {
        vramUnpacked.resize(dataSize);
    }

    // TODO: Crash on close!
    // Unpack VRAM to native GPU format (4x float)
    for (int y = 0; y < gpu::VRAM_HEIGHT; y++) {
        for (int x = 0; x < gpu::VRAM_WIDTH; x++) {
            PSXColor c = gpu->vram[y * gpu::VRAM_WIDTH + x];

            vramUnpacked[(y * gpu::VRAM_WIDTH + x) * 4 + 0] = floatLUT[c.r];
            vramUnpacked[(y * gpu::VRAM_WIDTH + x) * 4 + 1] = floatLUT[c.g];
            vramUnpacked[(y * gpu::VRAM_WIDTH + x) * 4 + 2] = floatLUT[c.b];
            vramUnpacked[(y * gpu::VRAM_WIDTH + x) * 4 + 3] = floatLUT[c.k * 31];
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

    int areaX = lastPos.x;
    int areaY = lastPos.y;
    int areaW = gpu->gp1_08.getHorizontalResoulution();
    int areaH = gpu->gp1_08.getVerticalResoulution();

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

#if 0
    const std::array<const char*, 15> blendModes= {{
            "GL_ZERO",
            "GL_ONE",
            "GL_SRC_COLOR",
            "GL_ONE_MINUS_SRC_COLOR",
            "GL_DST_COLOR",
            "GL_ONE_MINUS_DST_COLOR",
            "GL_SRC_ALPHA",
            "GL_ONE_MINUS_SRC_ALPHA",
            "GL_DST_ALPHA",
            "GL_ONE_MINUS_DST_ALPHA",
            "GL_CONSTANT_COLOR",
            "GL_ONE_MINUS_CONSTANT_COLOR",
            "GL_CONSTANT_ALPHA",
            "GL_ONE_MINUS_CONSTANT_ALPHA",
            "GL_SRC_ALPHA_SATURATE"
    }};

    auto mapBlendMode = [](int mode) {
        switch (mode){ 
            case 0:  return GL_ZERO;
            case 1:  return GL_ONE;
            case 2:  return GL_SRC_COLOR;
            case 3:  return GL_ONE_MINUS_SRC_COLOR;
            case 4:  return GL_DST_COLOR;
            case 5:  return GL_ONE_MINUS_DST_COLOR;
            case 6:  return GL_SRC_ALPHA;
            case 7:  return GL_ONE_MINUS_SRC_ALPHA;
            case 8:  return GL_DST_ALPHA;
            case 9:  return GL_ONE_MINUS_DST_ALPHA;
            case 10: return  GL_CONSTANT_COLOR;
            case 11: return  GL_ONE_MINUS_CONSTANT_COLOR;
            case 12: return  GL_CONSTANT_ALPHA;
            case 13: return  GL_ONE_MINUS_CONSTANT_ALPHA;
            case 14: return  GL_SRC_ALPHA_SATURATE;
        }
    };

    const std::array<const char*, 3> equations = {{
        "GL_FUNC_ADD",
        "GL_FUNC_SUBTRACT",
        "GL_FUNC_REVERSE_SUBTRACT"
    }};

    auto mapEquation = [](int mode) {
        switch (mode){ 
            case 0:  return GL_FUNC_ADD;
            case 1:  return GL_FUNC_SUBTRACT;
            case 2:  return GL_FUNC_REVERSE_SUBTRACT;
        }
    };

    static int selectedSrcA = 0;
    static int selectedSrcC = 0;
    static int selectedEquation = 0;
    static int selectedDstA = 0;
    static int selectedDstC = 0;

    static glm::vec4 blendColor;

    ImGui::Begin("BminusF");

    if (ImGui::Button("-##sa") && selectedSrcA > 0) selectedSrcA--; ImGui::SameLine();
    if (ImGui::Button("+##sa") && selectedSrcA < blendModes.size()-1) selectedSrcA++; ImGui::SameLine();
    ImGui::Combo("SRC A", &selectedSrcA, blendModes.data(), blendModes.size());


    if (ImGui::Button("-##sc") && selectedSrcC > 0) selectedSrcC--; ImGui::SameLine();
    if (ImGui::Button("+##sc") && selectedSrcC < blendModes.size()-1) selectedSrcC++; ImGui::SameLine();
    ImGui::Combo("SRC C", &selectedSrcC, blendModes.data(), blendModes.size());
    
    if (ImGui::Button("-##e") && selectedEquation > 0) selectedEquation--; ImGui::SameLine();
    if (ImGui::Button("+##e") && selectedEquation < equations.size()-1) selectedEquation++; ImGui::SameLine();
    ImGui::Combo("##equation", &selectedEquation, equations.data(), equations.size());
    
    if (ImGui::Button("-##da") && selectedDstA > 0) selectedDstA--; ImGui::SameLine();
    if (ImGui::Button("+##da") && selectedDstA < blendModes.size()-1) selectedDstA++; ImGui::SameLine();
    ImGui::Combo("DST A", &selectedDstA, blendModes.data(), blendModes.size());
    
    if (ImGui::Button("-##dc") && selectedDstC > 0) selectedDstC--; ImGui::SameLine();
    if (ImGui::Button("+##dc") && selectedDstC < blendModes.size()-1) selectedDstC++; ImGui::SameLine();
    ImGui::Combo("DST C", &selectedDstC, blendModes.data(), blendModes.size());

    ImGui::ColorPicker4("Blend Color", glm::value_ptr(blendColor));

    ImGui::End();
#endif

    // Unbatched render
    using Transparency = gpu::GP0_E1::SemiTransparency;

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    if (transparency) {
        glEnable(GL_BLEND);
    } else {
        glDisable(GL_BLEND);
    }
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

        if (transparency && buffer[i].flags & gpu::Vertex::SemiTransparency) {
            auto semi = static_cast<Transparency>((buffer[i].flags >> 5) & 3);
            if (semi == Transparency::Bby2plusFby2) {
                // Works ok, benchmark.exe
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                // glBlendFuncSeparate(mapBlendMode(selectedSrcC), mapBlendMode(selectedDstC), mapBlendMode(selectedSrcA),
                // mapBlendMode(selectedDstA)); glBlendEquation(mapEquation(selectedEquation)); glBlendColor(blendColor.r, blendColor.g,
                // blendColor.b, blendColor.a );
            } else if (semi == Transparency::BplusF) {
                // Works ok (Tekken 3 Sword glow)
                glBlendFunc(GL_DST_ALPHA, GL_ONE);
            } else if (semi == Transparency::BminusF) {
                // Ok on crash, black menu on Tekken3
                // glBlendEquation(GL_FUNC_REVERSE_SUBTRACT);

                glBlendFunc(GL_ONE, GL_ONE);

                // glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            } else if (semi == Transparency::BplusFby4) {
                glBlendFunc(GL_SRC_ALPHA, GL_ONE);
            }
        } else {
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        }

        glDrawArrays(type, i, count);

        i += count;
    }
    lastPos = glm::vec2(gpu->displayAreaStartX, gpu->displayAreaStartY);

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

    if (software) {
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
