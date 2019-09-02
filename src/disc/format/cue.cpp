#include "cue.h"

namespace disc {
namespace format {
std::string Cue::getFile() const { return file; }

Position Cue::getDiskSize() const {
    int frames = 0;
    for (auto t : tracks) {
        frames += t.frames;
    }
    return Position::fromLba(frames) + Position{0, 2, 0};
}

size_t Cue::getTrackCount() const { return tracks.size(); }

Position Cue::getTrackStart(int track) const {
    size_t total = 0;
    if (tracks.at(0).type == disc::TrackType::DATA) {
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

disc::Sector Cue::read(Position pos) {
    auto buffer = std::vector<uint8_t>(Track::SECTOR_SIZE);
    auto type = disc::TrackType::INVALID;

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

        if (trackNum == 0 && type == disc::TrackType::DATA) {
            pos = pos - Position{0, 2, 0};
        }
        auto seek = pos - *track.index0;
        fseek(file.get(), (long)(track.offset + seek.toLba() * Track::SECTOR_SIZE), SEEK_SET);
        fread(buffer.data(), Track::SECTOR_SIZE, 1, file.get());
    }

    return std::make_pair(buffer, type);
}

std::unique_ptr<Cue> Cue::fromBin(const char* file) {
    auto size = getFileSize(file);
    if (size == 0) {
        return {};
    }

    Track t;
    t.filename = file;
    t.number = 1;
    t.type = disc::TrackType::DATA;
    t.pregap = Position(0, 0, 0);
    t.index0 = Position(0, 0, 0);
    t.index1 = Position(0, 0, 0);
    t.offset = 0;
    t.frames = size / Track::SECTOR_SIZE;

    auto cue = std::make_unique<Cue>();
    cue->file = file;
    cue->tracks.push_back(t);

    cue->loadSubchannel(file);

    return cue;
}
}  // namespace format
}  // namespace disc
