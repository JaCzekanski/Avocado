#include "chd_format.h"
#include <fmt/core.h>
#include <cstring>
#include "disc/track.h"
#include "utils/file.h"

namespace disc {
namespace format {

std::unique_ptr<Chd> Chd::open(const std::string& path) {
    chd_file* chdFile;
    auto err = chd_open(path.c_str(), CHD_OPEN_READ, nullptr, &chdFile);
    if (err != CHDERR_NONE) {
        fmt::print("[CHD] Unable to load file {} (error: {})\n", path, err);
        return {};
    }

    // I'm not using make_unique - I want the constructor to be private
    auto chd = std::unique_ptr<Chd>(new Chd(path, chdFile));

    const chd_header* header = chd_get_header(chdFile);
    chd->hunkSize = header->hunkbytes;
    chd->lastHunk = std::vector<uint8_t>(chd->hunkSize);
    chd->lastHunkId = 0xfffffff;

    if ((chd->hunkSize % chd->sectorSize) != 0) {
        fmt::print("[CHD] Image uses invalid hunkSize: {}\n", chd->hunkSize);
        return {};
    }

    char metadata[512];

    for (;;) {
        int trackNum;
        char type[64];
        char subtype[64];
        int frames;
        int pregap = 0;
        char pgtype[32];
        char pgsub[32];
        int postgap = 0;

        if (chd_get_metadata(chdFile, CDROM_TRACK_METADATA2_TAG, chd->tracks.size(), metadata, sizeof(metadata), NULL, NULL, NULL)
            == CHDERR_NONE) {
            sscanf(metadata, CDROM_TRACK_METADATA2_FORMAT, &trackNum, type, subtype, &frames, &pregap, pgtype, pgsub, &postgap);
        } else if (chd_get_metadata(chdFile, CDROM_TRACK_METADATA_TAG, chd->tracks.size(), metadata, sizeof(metadata), NULL, NULL, NULL)
                   == CHDERR_NONE) {
            sscanf(metadata, CDROM_TRACK_METADATA_FORMAT, &trackNum, type, subtype, &frames);
        } else {
            if (chd->tracks.size() != 0) {
                break;
            }

            fmt::print("[CHD] No valid metadata found, are you using correct .chd file?\n");
            return {};
        }

        Track track;
        if (strcmp(type, "MODE2_RAW") == 0 || strcmp(type, "MODE2") == 0) {
            track.type = disc::TrackType::DATA;
        } else if (strcmp(type, "AUDIO") == 0) {
            track.type = disc::TrackType::AUDIO;
        } else {
            track.type = disc::TrackType::INVALID;
            fmt::print("[CHD] Unsupported track type {}\n", type);
            return {};
        }

        track.number = trackNum;
        track.frames = frames;
        track.pregap = Position::fromLba(pregap);
        track.index0 = Position::fromLba(0);
        track.index1 = Position::fromLba(pregap);
        track.postgap = Position::fromLba(postgap);

        chd->tracks.push_back(track);
    }

    chd->loadSubchannel(path);

    return chd;
}

Chd::Chd(std::string path, chd_file* chdFile) : path(path), chdFile(chdFile) {}

Chd::~Chd() { chd_close(chdFile); }

Sector Chd::read(Position pos) {
    int lba = (pos - Position{0, 2, 0}).toLba();
    size_t hunk = (lba * sectorSize) / hunkSize;
    size_t offset = (lba * sectorSize) % hunkSize;

    if (hunk != lastHunkId || lastHunk.empty()) {
        chd_read(chdFile, hunk, lastHunk.data());
        lastHunkId = hunk;
    }

    std::vector<uint8_t> data(Track::SECTOR_SIZE);
    std::copy(lastHunk.begin() + offset, lastHunk.begin() + offset + Track::SECTOR_SIZE, data.begin());

    disc::TrackType type = disc::TrackType::DATA;

    int trackN = getTrackByPosition(pos);
    if (trackN != -1) {
        type = tracks[trackN].type;
    }

    return std::make_pair(data, type);
}

std::string Chd::getFile() const { return path; }

size_t Chd::getTrackCount() const { return tracks.size(); }

int Chd::getTrackByPosition(Position pos) const {
    size_t posFrames = (pos - Position(0, 2, 0)).toLba();

    size_t frames = 0;
    for (size_t i = 0; i < tracks.size(); i++) {
        if (posFrames > frames && posFrames < frames + tracks[i].frames) {
            return i;
        }

        frames += tracks[i].frames;
    }

    return 0;
}

Position Chd::getTrackStart(int track) const {
    size_t frames = 0;
    if ((unsigned)track < tracks.size()) {
        for (int i = 0; i < track - 1; i++) {
            frames += tracks[i].frames;
        }
    }

    return Position::fromLba(frames) + Position(0, 2, 0);
}

Position Chd::getTrackLength(int track) const {
    if ((unsigned)track < tracks.size()) {
        return Position::fromLba(tracks[track].frames);
    } else {
        return Position(0, 2, 0);
    }
}

Position Chd::getDiskSize() const {
    size_t frames = 0;
    for (size_t i = 0; i < tracks.size(); i++) {
        frames += tracks[i].frames;
    }
    return Position::fromLba(frames) + Position(0, 2, 0);
}
}  // namespace format
}  // namespace disc