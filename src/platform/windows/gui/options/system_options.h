#pragma once
// Note: should be system.h, but it conflicts with main "system.h"

namespace gui::options {
class System {
   public:
    bool systemWindowOpen = false;

    void displayWindows();
};
};  // namespace gui::options