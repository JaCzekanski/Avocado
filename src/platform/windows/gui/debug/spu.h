#pragma once

struct System;

namespace spu {
struct SPU;
};

namespace gui::debug {
class SPU {
    bool showOpenDirectory = false;

    void spuWindow(spu::SPU* spu);

   public:
    bool spuWindowOpen = false;
    void displayWindows(System* sys);
    void recordingWindow(spu::SPU* spu);
};
}  // namespace gui::debug