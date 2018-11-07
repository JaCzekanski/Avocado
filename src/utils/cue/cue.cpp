#include "cue.h"

namespace utils {
Position Cue::getDiskSize() const {
    Position size = Position::fromLba(2 * 75);
    for (auto t : tracks) {
        // size = size + t.getTrackSize();
    }
    return size;
}

size_t Cue::getTrackCount() const { return tracks.size(); }

Position Cue::getTrackStart(int track) const { return {0, 2, 0}; }

Position Cue::getTrackLength(int track) const {
    //    return tracks.at(track).getTrackSize();
    return {0, 2, 0};
}

std::pair<std::vector<uint8_t>, Track::Type> Cue::read(Position pos, size_t bytes) {
    // 1. Iterate through tracks and find if pos >= track.start && pos < track.end
    // 2. Open file if not in cache
    // 3. read n bytes

    auto buffer = std::vector<uint8_t>(bytes);
    auto type = Track::Type::INVALID;

    pos = pos - Position{0, 2, 0};

    for (size_t i = 0; i < getTrackCount(); i++) {
        auto track = tracks[i];

        if (pos >= *track.index0 && pos >= *track.index0 + Position::fromLba(track.frames)) {
            if (files.find(track.filename) == files.end()) {
                FILE* f = fopen(track.filename.c_str(), "rb");
                if (!f) {
                    printf("Unable to load file %s\n", track.filename.c_str());
                    break;
                }

                files.emplace(track.filename, std::shared_ptr<FILE>(f, fclose));
            }

            type = track.type;
            auto file = files[track.filename];

            auto seek = pos - track.index1;
            fseek(file.get(), (long)(track.offset + seek.toLba() * Track::SECTOR_SIZE), SEEK_SET);
            fread(buffer.data(), bytes, 1, file.get());
            break;
        }
    }

    return std::make_tuple(buffer, type);
}

std::optional<Cue> Cue::fromBin(const char* file) {
    auto size = getFileSize(file);
    if (size == 0) {
        return {};
    }

    Track t;
    t.filename = file;
    t.number = 1;
    t.type = Track::Type::DATA;
    t.index1 = Position(0, 0, 0);

    auto cue = Cue();
    cue.file = file;
    cue.tracks.push_back(t);

    return cue;
}
}  // namespace utils
