#include "bios.h"
#include "config.h"

namespace gui::options {
Bios::Bios() {
    showOptions = false;
    windowName = "Select BIOS##file_dialog";
}

std::string Bios::getDefaultPath() { return "data/bios"; }

bool Bios::isFileSupported(const gui::helper::File& f) {
    constexpr std::array<const char*, 2> supportedFiles = {
        ".bin",  //
        ".rom",  //
    };

    if (std::find(supportedFiles.begin(), supportedFiles.end(), f.extension) == supportedFiles.end()) {
        return false;
    }

    auto size = fs::file_size(f.entry);
    if (size != 512 * 1024 && size != 256 * 1024) {
        return false;
    }

    return true;
}

bool Bios::onFileSelected(const gui::helper::File& f) {
    std::string biosPath = f.entry.path().string();

// SDL treats relative paths (not starting with /) as relative to app internal storage path
// Storing path as absolute is needed for file to be found
#ifdef ANDROID
    biosPath = fs::absolute(biosPath).string();
#endif
    config["bios"] = biosPath;
    bus.notify(Event::System::HardReset{});
    return true;
}

void Bios::displayWindows() {
    if (biosWindowOpen) {
        display(biosWindowOpen);
    }
}
};  // namespace gui::options