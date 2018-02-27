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

std::vector<uint8_t> Cue::read(Position& pos, size_t bytes, bool audio) {
    // 1. Iterate through tracks and find if pos >= track.start && pos < track.end
    // 2. Open file if not in cache
    // 3. read n bytes

    auto buffer = std::vector<uint8_t>();

    for (int i = 0; i < getTrackCount(); i++) {
        auto track = tracks[i];

        if (pos >= (track.start - track.pause) && pos < track.end) {
            if (audio && track.type != Track::Type::AUDIO) break;

            if (files.find(track.filename) == files.end()) {
                FILE* f = fopen(track.filename.c_str(), "rb");
                if (!f) {
                    printf("Unable to load file %s\n", track.filename.c_str());
                    break;
                }

                files.emplace(track.filename, std::shared_ptr<FILE>(f, fclose));
            }

            auto file = files[track.filename];

            auto seek = pos - track.start;
            fseek(file.get(), track.offsetInFile + seek.toLba() * 2352, SEEK_SET);

            buffer.resize(bytes);
            fread(buffer.data(), bytes, 1, file.get());
            break;
        }
    }

    return buffer;
}

std::unique_ptr<Cue> Cue::fromBin(const char* file) {
    auto size = getFileSize(file);
    if (size == 0) return nullptr;

    Track t;
    t.filename = file;
    t.number = 1;
    t.offsetInFile = 0;
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
}  // namespace utils
