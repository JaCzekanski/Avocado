#include "open.h"
#include "config.h"

namespace gui::file {
Open::Open() { windowName = "Open file##file_dialog"; }

bool Open::isFileSupported(const gui::helper::File& f) {
    constexpr std::array<const char*, 9> supportedFiles = {
        ".iso",      //
        ".cue",      //
        ".bin",      //
        ".img",      //
        ".chd",      //
        ".exe",      //
        ".psexe",    //
        ".psf",      //
        ".minipsf",  //
    };

    return std::find(supportedFiles.begin(), supportedFiles.end(), f.extension) != supportedFiles.end();
}

bool Open::onFileSelected(const gui::helper::File& f) {
    auto path = f.entry.path();
    config["gui"]["lastPath"] = path.parent_path().string();

    bus.notify(Event::File::Load{path.string(), true});
    return true;
}

void Open::displayWindows() {
    if (openWindowOpen) {
        display(openWindowOpen);
    }
}
};  // namespace gui::file