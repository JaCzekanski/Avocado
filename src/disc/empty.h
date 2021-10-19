#pragma once
#include "disc.h"

namespace disc {

struct Empty : public Disc {
    ~Empty() = default;

    Sector read(Position pos) {
        (void)pos;
        return std::make_pair(std::vector<uint8_t>(), TrackType::INVALID);
    }

    std::string getFile() const { return ""; }

    size_t getTrackCount() const { return 0; }

    int getTrackByPosition(Position pos) const {
        (void)pos;
        return 0;
    }

    Position getTrackBegin(int track) const {
        (void)track;
        return Position{0, 0, 0};
    }

    Position getTrackStart(int track) const {
        (void)track;
        return Position{0, 0, 0};
    }

    Position getTrackLength(int track) const {
        (void)track;
        return Position{0, 0, 0};
    }

    Position getDiskSize() const { return Position{0, 0, 0}; }
};
}  // namespace disc