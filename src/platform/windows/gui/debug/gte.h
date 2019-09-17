#pragma once

struct System;

namespace gui::debug {
class GTE {
    void logWindow(System* sys);
    void registersWindow(System* sys);

   public:
    bool logWindowOpen = false;
    bool registersWindowOpen = false;
    void displayWindows(System* sys);
};
}  // namespace gui::debug