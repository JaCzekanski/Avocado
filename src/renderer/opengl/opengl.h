#pragma once
#include <glad/glad.h>
#include <memory>
#include "device/gpu/gpu.h"
#include "shader/Program.h"

class OpenGL {
   public:
    static const int resWidth = 640;
    static const int resHeight = 480;

    int width = resWidth;
    int height = resHeight;

    bool init();
    bool setup();
    void render(GPU* gpu);

    void setViewFullVram(bool v) { viewFullVram = v; }

    bool getViewFullVram() { return viewFullVram; }

    // Debug
    GLuint getVramTextureId() const { return renderTex; }

   private:
    struct BlitStruct {
        float pos[2];
        float tex[2];
    };

    const int bufferSize = 1024 * 1024;

    std::unique_ptr<Program> renderShader;
    std::unique_ptr<Program> blitShader;

    // VRAM to screen blit
    GLuint blitVao = 0;
    GLuint blitVbo = 0;

    GLuint renderTex = 0;

    bool viewFullVram = false;

    bool loadExtensions();
    bool loadShaders();
    void createBlitBuffer();
    void createVramTexture();
    void createRenderTexture();
    void updateTextureParameters();
    std::vector<BlitStruct> makeBlitBuf(int screenX = 0, int screenY = 0, int screenW = 640, int screenH = 480);
    void renderSecondStage();
};