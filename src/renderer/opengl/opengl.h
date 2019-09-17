#pragma once
#include <opengl.h>
#include <memory>
#include "device/gpu/gpu.h"
#include "shader/buffer.h"
#include "shader/framebuffer.h"
#include "shader/program.h"
#include "shader/texture.h"
#include "shader/vertex_array_object.h"

class OpenGL {
   public:
#ifdef USE_OPENGLES
    const int VERSION_MAJOR = 3;
    const int VERSION_MINOR = 0;
#else
    const int VERSION_MAJOR = 3;
    const int VERSION_MINOR = 2;
#endif

    static const int resWidth = 640;
    static const int resHeight = 480;

    const float RATIO_4_3 = 4.f / 3.f;
    const float RATIO_16_9 = 16.f / 9.f;

    int width = resWidth;
    int height = resHeight;
    float aspect = RATIO_4_3;

    OpenGL();
    ~OpenGL();
    bool setup();
    void render(gpu::GPU* gpu);

   private:
    int busToken = -1;
    struct BlitStruct {
        float pos[2];
        float tex[2];
    };

    const int bufferSize = 10000;

    bool hardwareRendering;
    bool transparency;

    std::unique_ptr<VertexArrayObject> vao;
    std::unique_ptr<Program> renderShader;
    std::unique_ptr<Buffer> renderBuffer;
    std::unique_ptr<Framebuffer> renderFramebuffer;
    std::unique_ptr<Texture> renderTex;
    std::unique_ptr<Texture> vramTex;
    std::unique_ptr<Texture> vram24Tex;
    bool supportNativeTexture;

    int renderWidth;
    int renderHeight;

    // VRAM to screen blit
    std::unique_ptr<Program> blitShader;
    std::unique_ptr<Buffer> blitBuffer;

    std::unique_ptr<Program> copyShader;

    bool loadExtensions();
    bool loadShaders();
    void bindRenderAttributes();
    void renderVertices(gpu::GPU* gpu);

    std::vector<uint8_t> vram24Unpacked;
    std::vector<uint16_t> vramUnpacked;
    void update24bitTexture(gpu::GPU* gpu);
    void updateVramTexture(gpu::GPU* gpu);

    void bindBlitAttributes();
    std::vector<BlitStruct> makeBlitBuf(int screenX = 0, int screenY = 0, int screenW = 640, int screenH = 480, bool invert = false);
    void renderBlit(gpu::GPU* gpu, bool software);

    void bindCopyAttributes();
};
