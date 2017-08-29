#pragma once
#include "position.h"
#include "track.h"
#include <vector>

namespace utils {
struct Cue {
    std::string file;
    std::vector<Track> tracks;

    Position getDiskSize() const;
    int getTrackCount() const;
    Position getTrackLength(int track) const;

    void seekTo();
    // read method
};
}
