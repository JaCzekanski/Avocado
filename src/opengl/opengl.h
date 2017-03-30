#pragma once
#include <externals/glad/include/glad/glad.h>
#include "shader/Program.h"
#include <memory>

namespace device {
namespace gpu {
class GPU;
}
}

class OpenGL {
   public:
    struct Vertex {
        int position[2];
        int color[3];
        int texcoord[2];
        int bitcount;
        int clut[2];     // clut position
        int texpage[2];  // texture page position
    };

    static const int resWidth = 640;
    static const int resHeight = 480;

    bool init();
    bool setup();
    void render(device::gpu::GPU* gpu);

    void setViewFullVram(bool v) { viewFullVram = v; }

    bool getViewFullVram() { return viewFullVram; }

   private:
    struct BlitStruct {
        float pos[2];
        float tex[2];
    };

    const int bufferSize = 1024 * 1024;

    std::unique_ptr<Program> renderShader;
    std::unique_ptr<Program> blitShader;

    // emulated console GPU calls
    GLuint renderVao = 0;  // attributes
    GLuint renderVbo = 0;  // buffer

    // VRAM to screen blit
    GLuint blitVao = 0;
    GLuint blitVbo = 0;

    GLuint vramTex = 0;
    GLuint renderTex = 0;
    GLuint framebuffer = 0;

    bool viewFullVram = false;

    bool loadExtensions();
    bool loadShaders();
    void createRenderBuffer();
    void createBlitBuffer();
    void createVramTexture();
    void createRenderTexture();
    std::vector<BlitStruct> makeBlitBuf(int screenX = 0, int screenY = 0, int screenW = 640, int screenH = 480);
    void renderFirstStage(const std::vector<Vertex>& renderList, device::gpu::GPU* gpu);
    void renderSecondStage();
};