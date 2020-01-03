#pragma once
#include "platform/windows/gui/helper/file_dialog.h"

namespace gui::file {
class Open : public gui::helper::FileDialog {
    bool isFileSupported(const gui::helper::File& f) override;
    bool onFileSelected(const gui::helper::File& f) override;

   public:
    bool openWindowOpen = false;

    Open();
    void displayWindows();
};
};  // namespace gui::file