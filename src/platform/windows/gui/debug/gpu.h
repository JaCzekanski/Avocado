#pragma once

struct System;

namespace gui::debug {
class GPU {
    void registersWindow(System *sys);
    void logWindow(System *sys);

   public:
    bool registersWindowOpen = false;
    bool logWindowOpen = false;

    void displayWindows(System *sys);
}
}  // namespace gui::debug