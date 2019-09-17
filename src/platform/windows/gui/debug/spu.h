#pragma once

struct System;

namespace spu {
struct SPU;
};

namespace gui::debug {
class SPU {
    void spuWindow(spu::SPU* spu);

   public:
    bool spuWindowOpen = false;
    void displayWindows(System* sys);
};
}  // namespace gui::debug