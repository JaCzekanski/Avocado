#pragma once

struct System;

namespace gui::debug {
class Cdrom {
    bool useFrames = false;
    void cdromWindow(System* sys);

   public:
    bool cdromWindowOpen = false;
    void displayWindows(System* sys);
};
}  // namespace gui::debug::cdrom