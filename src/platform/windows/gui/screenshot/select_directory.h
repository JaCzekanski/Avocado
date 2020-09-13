#pragma once
#include "platform/windows/gui/helper/file_dialog.h"

namespace gui::screenshot {
using namespace gui::helper;
class SelectDirectory : public FileDialog {
    bool onFileSelected(const File& f) override;

   public:
    bool windowOpen = false;

    SelectDirectory();
    void displayWindows();
};
};  // namespace gui::file