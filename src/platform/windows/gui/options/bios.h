#pragma once
#include "platform/windows/gui/helper/file_dialog.h"

namespace gui::options {
class Bios : public gui::helper::FileDialog {
    std::string getDefaultPath() override;
    bool isFileSupported(const gui::helper::File& f) override;
    bool onFileSelected(const gui::helper::File& f) override;

   public:
    bool biosWindowOpen = false;

    Bios();
    void displayWindows();
};
};  // namespace gui::options