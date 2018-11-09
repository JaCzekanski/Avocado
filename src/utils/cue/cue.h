#pragma once
#include <cstdio>
#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>
#include "position.h"
#include "track.h"
#include "utils/file.h"

namespace utils {
struct Cue {
    std::string file;
    std::vector<Track> tracks;

    Position getDiskSize() const;
    size_t getTrackCount() const;
    Position getTrackStart(int track) const;
    Position getTrackLength(int track) const;
    int getTrackByPosition(Position pos) const;

    void seekTo();
    std::pair<std::vector<uint8_t>, Track::Type> read(Position pos, size_t bytes = 2352);
    // read method

    static std::optional<Cue> fromBin(const char* file);

   private:
    std::unordered_map<std::string, std::shared_ptr<FILE>> files;
};
}  // namespace utils
