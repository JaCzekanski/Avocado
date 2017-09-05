#pragma once
#include "position.h"
#include "track.h"
#include "utils/file.h"
#include <vector>
#include <memory>

namespace utils {
struct Cue {
    std::string file;
    std::vector<Track> tracks;

    Position getDiskSize() const;
    int getTrackCount() const;
    Position getTrackLength(int track) const;

    void seekTo();
    // read method

    static std::unique_ptr<Cue> fromBin(const char* file);
};
}
