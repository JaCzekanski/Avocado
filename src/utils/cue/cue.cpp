#include "cue.h"

namespace utils {
Position Cue::getDiskSize() const {
    Position size = Position::fromLba(2 * 75);
    for (auto t : tracks) {
        size = size + t.getTrackSize();
    }
    return size;
}

int Cue::getTrackCount() const { return tracks.size(); }

Position Cue::getTrackLength(int track) const { return tracks.at(track).getTrackSize(); }
}
