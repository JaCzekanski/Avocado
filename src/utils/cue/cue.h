#pragma once
#include <cstdio>
#include <memory>
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
    Position getTrackLength(int track) const;

    void seekTo();
    std::vector<uint8_t> read(Position& pos, size_t bytes, bool audio = false);
    // read method

    static std::unique_ptr<Cue> fromBin(const char* file);

   private:
    std::unordered_map<std::string, std::shared_ptr<FILE>> files;
};
}  // namespace utils
