#pragma once
#include "position.h"

namespace utils {
enum class TrackType { DATA, AUDIO };

struct Track {
    std::string filename;
    int number = 0;
    TrackType type;

    // Pregap is not included
    // Pause is included in image

    Position pregap;  // size, not position
    Position pause;
    Position start;
    Position end;

    size_t size;
    Position _index0;  // not used

    Position getTrackSize() const;
};
}
