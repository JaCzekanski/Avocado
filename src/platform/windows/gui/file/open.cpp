#include "open.h"
#include "config.h"

namespace gui::file {
Open::Open() : FileDialog(Mode::OpenFile) { windowName = "Open file##file_dialog"; }

bool Open::isFileSupported(const gui::helper::File& f) {
    constexpr std::array<const char*, 10> supportedFiles = {
        ".iso",      //
        ".cue",      //
        ".bin",      //
        ".img",      //
        ".chd",      //
        ".ecm",      //
        ".exe",      //
        ".psexe",    //
        ".psf",      //
        ".minipsf",  //
    };

    return std::find(supportedFiles.begin(), supportedFiles.end(), f.extension) != supportedFiles.end();
}

bool Open::onFileSelected(const gui::helper::File& f) {
    auto path = f.entry.path();
    config.gui.lastPath = path.parent_path().string();

    bus.notify(Event::File::Load{path.string()});
    return true;
}

void Open::displayWindows() {
    if (openWindowOpen) {
        display(openWindowOpen);
    }
}
};  // namespace gui::file