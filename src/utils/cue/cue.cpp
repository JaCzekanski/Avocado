#include "cue.h"

namespace utils {
Position Cue::getDiskSize() const {
    Position size = Position::fromLba(2 * 75);
    for (auto t : tracks) {
        size = size + t.getTrackSize();
    }
    return size;
}

int Cue::getTrackCount() const { return tracks.size(); }

Position Cue::getTrackLength(int track) const { return tracks.at(track).getTrackSize(); }

std::unique_ptr<Cue> Cue::fromBin(const char* file) {
    auto size = getFileSize(file);
    if (size == 0) return nullptr;

    Track t;
    t.filename = file;
    t.number = 1;
    t.size = size;
    t.type = Track::Type::DATA;
    t.start = Position(0, 0, 0);
    t.end = Position::fromLba(size / Track::SECTOR_SIZE);
    t.pause = Position(0, 0, 0);

    auto cue = std::make_unique<Cue>();
    cue->file = file;
    cue->tracks.push_back(t);

    return cue;
}
}
