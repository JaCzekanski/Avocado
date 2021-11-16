#include "system_tools.h"
#include <fmt/core.h>
#include "config.h"
#include "disc/load.h"
#include "sound/sound.h"
#include "state/state.h"
#include "system.h"
#include "utils/file.h"
#include "utils/gpu_draw_list.h"
#include "utils/psf.h"

namespace system_tools {

void bootstrap(std::unique_ptr<System>& sys) {
    Sound::clearBuffer();
    sys = hardReset();

    // Breakpoint on BIOS Shell execution
    sys->cpu->addBreakpoint(0x80030000);

    // Execute BIOS till breakpoint hit (shell is about to be executed)
    while (sys->state == System::State::run) sys->emulateFrame();
}

void loadFile(std::unique_ptr<System>& sys, const std::string& path) {
    std::string ext = getExtension(path);
    transform(ext.begin(), ext.end(), ext.begin(), tolower);

    std::string filenameExt = getFilenameExt(path);
    transform(filenameExt.begin(), filenameExt.end(), filenameExt.begin(), tolower);

    if (ext == "psf" || ext == "minipsf") {
        bootstrap(sys);
        if (loadPsf(sys.get(), path)) {
            toast(fmt::format("{} loaded", filenameExt));
        } else {
            toast(fmt::format("Cannot load {}", filenameExt));
        }
        sys->state = System::State::run;
        return;
    }

    if (ext == "exe" || ext == "psexe") {
        bool isPaused = sys->state == System::State::pause;
        bootstrap(sys);
        // Replace shell with .exe contents
        if (sys->loadExeFile(getFileContents(path))) {
            toast(fmt::format("{} loaded", filenameExt));
        } else {
            toast(fmt::format("Cannot load {}", filenameExt));
        }

        // Resume execution
        sys->state = isPaused ? System::State::pause : System::State::run;
        return;
    }

    if (ext == "state") {
        if (state::loadFromFile(sys.get(), path)) {
            sys->state = System::State::pause;
            return;
        }
    }

    if (ext == "gpudrawlist") {
        if (GpuDrawList::load(sys.get(), path)) {
            sys->state = System::State::pause;
            GpuDrawList::replayCommands(sys->gpu.get());
            toast(fmt::format("{} loaded", filenameExt));
            bus.notify(Event::Gui::Debug::OpenDrawListWindows{});
            return;
        }
    }

    std::unique_ptr<disc::Disc> disc = disc::load(path);
    if (disc) {
        sys->cdrom->setShell(true);
        sys->cdrom->disc = std::move(disc);
        sys->cdrom->setShell(false);
        toast(fmt::format("{} loaded", filenameExt));
    } else {
        toast(fmt::format("Cannot load {}", filenameExt));
    }
}

void saveMemoryCards(std::unique_ptr<System>& sys, bool force) {
    auto saveMemoryCard = [&](int slot) {
        if (!force && !sys->controller->card[slot]->dirty) return;

    std::string pathCard = config.memoryCard[slot].path;

    if (pathCard.empty()) {
        fmt::print("[INFO] No memory card {} path in config, skipping save\n", slot + 1);
            return;
    }

        auto& data = sys->controller->card[slot]->data;
        auto output = std::vector<uint8_t>(data.begin(), data.end());

        if (!putFileContents(pathCard, output)) {
            fmt::print("[INFO] Unable to save memory card {} to {}\n", slot + 1, getFilenameExt(pathCard));
            return;
    }

    fmt::print("[INFO] Saved memory card {} to {}\n", slot + 1, getFilenameExt(pathCard));
};

    saveMemoryCard(0);
    saveMemoryCard(1);
    }

std::unique_ptr<System> hardReset() {
    auto sys = std::make_unique<System>();

    std::string bios = config.bios;
    if (!bios.empty() && sys->loadBios(bios)) {
        fmt::print("[INFO] Using bios {}\n", getFilenameExt(bios));
    }

    std::string extension = config.extension;
    if (!extension.empty() && sys->loadExpansion(getFileContents(extension))) {
        fmt::print("[INFO] Using extension {}\n", getFilenameExt(extension));
    }

    std::string iso = config.iso;
    if (!iso.empty()) {
        loadFile(sys, iso);
        fmt::print("[INFO] Using iso {}\n", iso);
    }

    auto loadMemoryCard = [&](int slot) {
        assert(slot == 0 || slot == 1);
        auto card = sys->controller->card[slot].get();

        std::string pathCard = config.memoryCard[slot].path;

        card->inserted = false;

        if (!pathCard.empty()) {
            auto cardData = getFileContents(pathCard);
            if (!cardData.empty()) {
                std::copy_n(std::make_move_iterator(cardData.begin()), cardData.size(), sys->controller->card[slot]->data.begin());
                card->inserted = true;
                fmt::print("[INFO] Loaded memory card {} from {}\n", slot + 1, getFilenameExt(pathCard));
            }
        }
    };

    loadMemoryCard(0);
    loadMemoryCard(1);

    return sys;
}

};  // namespace system_tools
