#include "system_tools.h"
#include <fmt/core.h>
#include "config.h"
#include "disc/load.h"
#include "sound/sound.h"
#include "state/state.h"
#include "system.h"
#include "memory_card/card_formats.h"
#include "utils/file.h"
#include "utils/gpu_draw_list.h"
#include "utils/psf.h"
#include "utils/gameshark.h"

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

    if (ext == "txt") {
        Gameshark* gameshark = Gameshark::getInstance();
        gameshark->readFile(path);
        return;
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

bool saveMemoryCard(std::unique_ptr<System>& sys, int slot, bool force) {
    if (!force && !sys->controller->card[slot]->dirty) return true;

    std::string pathCard = config.memoryCard[slot].path;

    if (pathCard.empty()) {
        fmt::print("[INFO] No memory card {} path in config, skipping save\n", slot + 1);
        return false;
    }

    if (!memory_card::save(sys->controller->card[slot]->data, pathCard)) {
        fmt::print("[ERROR] Unable to save memory card {} to {}\n", slot + 1, getFilenameExt(pathCard));
        return false;
    }
    sys->controller->card[slot]->dirty = false;

    fmt::print("[INFO] Saved memory card {} to {}\n", slot + 1, getFilenameExt(pathCard));
    return true;
};

bool loadMemoryCard(std::unique_ptr<System>& sys, int slot) {
    assert(slot == 0 || slot == 1);
    auto card = sys->controller->card[slot].get();
    card->inserted = false;

    auto path = config.memoryCard[slot].path;
    if (path.empty()) {
        return false;
    }

    auto cardData = memory_card::load(path);
    if (!cardData) {
        fmt::print("[ERROR] Cannot load memory card {} ({})\n", slot, getFilenameExt(path));
        return false;
    }

    std::copy(cardData->begin(), cardData->end(), sys->controller->card[slot]->data.begin());
    card->inserted = true;
    card->setFresh();

    fmt::print("[INFO] Loaded memory card {} from {}\n", slot + 1, getFilenameExt(path));
    return true;
};

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

    loadMemoryCard(sys, 0);
    loadMemoryCard(sys, 1);

    return sys;
}

};  // namespace system_tools
