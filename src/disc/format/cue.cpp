#include "cue.h"
#include <fmt/core.h>

namespace disc {
namespace format {
std::string Cue::getFile() const { return file; }

Position Cue::getDiskSize() const {
    size_t frames = 75 * 2;
    for (auto t : tracks) {
        frames += t.pregap.toLba() + t.frames;
    }
    return Position::fromLba(frames);
}

size_t Cue::getTrackCount() const { return tracks.size(); }

Position Cue::getTrackBegin(int track) const {
    size_t total = 75 * 2;
    for (int i = 0; i < track; i++) {
        total += tracks[i].pregap.toLba() + tracks[i].frames;
    }
    return Position::fromLba(total);
}

Position Cue::getTrackStart(int track) const { return getTrackBegin(track) + tracks[track].start(); }

Position Cue::getTrackLength(int track) const { return Position::fromLba(tracks[track].pregap.toLba() + tracks[track].frames); }

int Cue::getTrackByPosition(Position pos) const {
    for (size_t i = 0; i < getTrackCount(); i++) {
        auto begin = getTrackBegin(i);
        auto length = getTrackLength(i);

        if (pos >= begin && pos < begin + length) {
            return i;
        }
    }
    return -1;
}

disc::Sector Cue::read(Position pos) {
    auto buffer = std::vector<uint8_t>(Track::SECTOR_SIZE, 0);
    auto trackNum = getTrackByPosition(pos);
    if (trackNum == -1) {
        return std::make_pair(buffer, disc::TrackType::INVALID);
    }

    auto track = tracks[trackNum];
    if (files.find(track.filename) == files.end()) {
        auto f = unique_ptr_file(fopen(track.filename.c_str(), "rb"));
        if (!f) {
            fmt::print("Unable to load file {}\n", track.filename);
            return std::make_pair(buffer, disc::TrackType::INVALID);
        }

        files.emplace(track.filename, std::move(f));
    }
    auto file = files[track.filename].get();
    auto seek = pos - (getTrackBegin(trackNum) + track.pregap);

    long offset = track.offset + seek.toLba() * Track::SECTOR_SIZE;
    if (offset < 0) {  // Pregap
        return std::make_pair(buffer, track.type);
    }

    fseek(file, offset, SEEK_SET);
    fread(buffer.data(), Track::SECTOR_SIZE, 1, file);

    return std::make_pair(buffer, track.type);
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
