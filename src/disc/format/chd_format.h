#pragma once
#include "chd.h"
#include "disc/disc.h"
#include "disc/track.h"

namespace disc {
namespace format {
struct Chd : public Disc {
    const int sectorSize = Track::SECTOR_SIZE + 96;  // .chd file stores subcode as well, though it is empty

    static std::unique_ptr<Chd> open(const std::string& path);

    ~Chd() override;

    Sector read(Position pos) override;

    std::string getFile() const override;
    size_t getTrackCount() const override;
    int getTrackByPosition(Position pos) const override;
    Position getTrackStart(int track) const override;
    Position getTrackLength(int track) const override;
    Position getDiskSize() const override;

   private:
    Chd(std::string path, chd_file* chdFile);

    std::string path;
    chd_file* chdFile;
    std::vector<Track> tracks;

    size_t hunkSize;
    size_t lastHunkId = 0xFFFFFFF;
    std::vector<uint8_t> lastHunk;
};
}  // namespace format
}  // namespace disc