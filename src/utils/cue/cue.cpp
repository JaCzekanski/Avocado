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

Position Cue::getTrackStart(int track) const {
    size_t total = 0;
    if (tracks.at(0).type == Track::Type::DATA) {
        total += 75 * 2;
    }
    for (int i = 0; i < track - 1; i++) {
        total += tracks.at(i).frames;
    }
    return Position::fromLba(total);
}

Position Cue::getTrackLength(int track) const { return Position::fromLba(tracks.at(track).frames); }

int Cue::getTrackByPosition(Position pos) const {
    for (size_t i = 0; i < getTrackCount(); i++) {
        auto start = getTrackStart(i);
        auto size = tracks[i].frames;

        if (pos >= start && pos < start + Position::fromLba(size)) {
            return i;
        }
    }
    return -1;
}

std::pair<std::vector<uint8_t>, Track::Type> Cue::read(Position pos, size_t bytes) {
    auto buffer = std::vector<uint8_t>(bytes);
    auto type = Track::Type::INVALID;

    // pos = pos;// - Position{0, 2, 0};

    auto trackNum = getTrackByPosition(pos);
    if (trackNum != -1) {
        auto track = tracks[trackNum];
        if (files.find(track.filename) == files.end()) {
            FILE* f = fopen(track.filename.c_str(), "rb");
            if (!f) {
                printf("Unable to load file %s\n", track.filename.c_str());
                return std::make_pair(buffer, type);
            }

            files.emplace(track.filename, std::shared_ptr<FILE>(f, fclose));
        }

        type = track.type;
        auto file = files[track.filename];

        if (trackNum == 0 && type == Track::Type::DATA) {
            pos = pos - Position{0, 2, 0};
        }
        auto seek = pos - *track.index0;
        fseek(file.get(), (long)(track.offset + seek.toLba() * Track::SECTOR_SIZE), SEEK_SET);
        fread(buffer.data(), bytes, 1, file.get());
    }

    return std::make_pair(buffer, type);
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
    t.pregap = Position(0, 0, 0);
    t.index0 = Position(0, 0, 0);
    t.index1 = Position(0, 0, 0);
    t.offset = 0;
    t.frames = size / Track::SECTOR_SIZE;

    auto cue = Cue();
    cue.file = file;
    cue.tracks.push_back(t);

    return cue;
}
}  // namespace utils
