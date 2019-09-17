#pragma once

namespace gui::help {
class About {
    void aboutWindow();

   public:
    bool aboutWindowOpen = false;

    void displayWindows();
};
}  // namespace gui::help