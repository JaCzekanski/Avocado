#pragma once
#include <optional>
#include "position.h"

namespace utils {
struct Track {
    static const int SECTOR_SIZE = 2352;
    enum class Type { DATA, AUDIO, INVALID };

    std::string filename;
    int number = 0;
    Type type;

    Position pregap;                 // (Size) silence not included in image
    std::optional<Position> index0;  // (Position) Audio "before" start, same as pause
    Position index1;                 // (Position) Start of data/audio
    Position postgap;                // (Size) "silence" included in image

    size_t offset;  // aka offset in file in sectors (frames)
    size_t frames;  // aka size in sectors

    Track() {
        pregap = {0, 0, 0};
        index1 = {0, 0, 0};
        postgap = {0, 0, 0};

        frames = 0;
    }
};
}  // namespace utils
