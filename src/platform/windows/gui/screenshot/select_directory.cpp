#include "select_directory.h"
#include "config.h"

namespace gui::screenshot {
SelectDirectory::SelectDirectory() : FileDialog(Mode::SelectDirectory) { windowName = "Save 3D screenshot##select_directory"; }

bool SelectDirectory::onFileSelected(const File& f) {
    std::string value = f.entry.path().string();
    config.gui.lastPath = value;
    bus.notify(Event::Screenshot::Save{value, true});
    return true;
}

void SelectDirectory::displayWindows() {
    if (windowOpen) {
        display(windowOpen);
    }
}
};  // namespace gui::screenshot