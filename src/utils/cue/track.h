#pragma once
#include "position.h"

namespace utils {
struct Track {
    static const int SECTOR_SIZE = 2352;
    enum class Type { DATA, AUDIO };

    std::string filename;
    int number = 0;
    Type type;

    // Pregap is not included
    // Pause is included in image

    Position pregap;  // Length, not included in image
    Position pause;   // Length, included in image
    Position start;
    Position end;

    size_t offsetInFile = 0;
    size_t size;

    Position getTrackSize() const;
};
}  // namespace utils
