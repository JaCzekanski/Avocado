#include "state.h"
#include <fmt/core.h>
#include <cereal/archives/binary.hpp>
#include <cereal/types/array.hpp>
#include <cereal/types/deque.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/utility.hpp>
#include <cereal/types/vector.hpp>
#include <chrono>
#include <deque>
#include <sstream>
#include "config.h"
#include "disc/load.h"
#include "system.h"
#include "utils/file.h"

using namespace std::chrono_literals;

namespace state {
const char* lastSaveName = "last.state";

struct StateMetadata {
    inline static const uint32_t SAVESTATE_VERSION = 8;

    uint32_t version = SAVESTATE_VERSION;
    std::string biosPath;
    std::string discPath;

    // Extension is not saved
    // Controller state is not saved
    // memcard state is not saved

    System* sys;

    StateMetadata(System* sys) : sys(sys) {
        biosPath = sys->biosPath;
        if (sys->cdrom->disc) {
            discPath = sys->cdrom->disc->getFile();
        }
    }

    template <class Archive>
    void save(Archive& ar) const {
        ar(version);
        ar(biosPath);
        ar(discPath);

        ar(*sys);
    }

    template <class Archive>
    void load(Archive& ar) {
        ar(version);
        if (version != SAVESTATE_VERSION) {
            throw std::runtime_error(fmt::format("Incompatible save state version {} (expected {})", version, SAVESTATE_VERSION));
        }

        ar(biosPath);
        ar(discPath);

        ar(*sys);

        if (!biosPath.empty() && biosPath != sys->biosPath) {
            sys->loadBios(biosPath);
        }

        if (!discPath.empty() && discPath != sys->cdrom->disc->getFile()) {
            std::unique_ptr<disc::Disc> disc = disc::load(discPath);
            if (!disc) {
                sys->cdrom->setShell(true);
                toast(fmt::format("Cannot load {}", discPath));
            } else {
                sys->cdrom->disc = std::move(disc);
            }
        }
    }
};

SaveState save(System* sys) {
    std::ostringstream oos;
    cereal::BinaryOutputArchive archive(oos);

    StateMetadata metadata(sys);
    archive(metadata);

    return oos.str();
}

bool load(System* sys, const SaveState& state) {
    std::istringstream iss(state);
    cereal::BinaryInputArchive archive(iss);

    StateMetadata metadata(sys);

    try {
        archive(metadata);
    } catch (std::exception& e) {
        toast("Incompatible save state version");
        sys->state = System::State::halted;
        return false;
    }
    return true;
}

bool saveToFile(System* sys, const std::string& path) {
    auto state = state::save(sys);
    if (state.empty()) {
        return false;
    }
    return putFileContents(path, state);
}

bool loadFromFile(System* sys, const std::string& path) {
    auto state = getFileContentsAsString(path);
    if (state.empty()) {
        return false;
    }

    return state::load(sys, state);
}

std::string getStatePath(System* sys, int slot) {
    std::string name;
    if (sys->cdrom->disc) {
        std::string discPath = sys->cdrom->disc->getFile();
        if (!discPath.empty()) {
            name = getFilename(discPath);
        }
    }

    if (name.empty()) {
        name = getFilename(sys->biosPath);
    }

    return avocado::statePath(fmt::format("{}_{}.state", name, slot).c_str());
}

void quickSave(System* sys, int slot) {
    // TODO: Make directories!
    auto path = getStatePath(sys, slot);
    auto state = state::save(sys);
    if (putFileContents(path, state)) {
        toast(fmt::format("State {} saved", slot));
    } else {
        toast(fmt::format("Cannot save state {}", slot));
    }
}

void quickLoad(System* sys, int slot) {
    auto path = getStatePath(sys, slot);
    auto state = getFileContentsAsString(path);
    if (state.empty()) {
        toast(fmt::format("Cannot load state {}", slot));
        return;
    }
    if (state::load(sys, state)) {
        toast(fmt::format("State {} loaded", slot));
    }
}

bool saveLastState(System* sys) { return saveToFile(sys, avocado::statePath(lastSaveName)); }

bool loadLastState(System* sys) { return loadFromFile(sys, avocado::statePath(lastSaveName)); }

const auto timeTravelInterval = 1s;
const int timeTravelHistorySize = 60;  // 1 minute of state history
auto lastTime = std::chrono::steady_clock::now();
std::deque<std::string> timeTravel;

void manageTimeTravel(System* sys) {
    auto now = std::chrono::steady_clock::now();
    if (now - lastTime >= timeTravelInterval) {
        lastTime = now;

        bool timeTravelEnabled = config.options.emulator.timeTravel;
        if (!timeTravelEnabled) {
            return;
        }

        timeTravel.push_front(state::save(sys));

        if (timeTravel.size() > timeTravelHistorySize) {
            timeTravel.erase(timeTravel.begin() + timeTravelHistorySize);
        }
    }
}

bool rewindState(System* sys) {
    if (timeTravel.empty()) {
        return false;
    }
    lastTime = std::chrono::steady_clock::now();

    bool result = state::load(sys, std::move(timeTravel.front()));
    timeTravel.pop_front();

    return result;
}

};  // namespace state