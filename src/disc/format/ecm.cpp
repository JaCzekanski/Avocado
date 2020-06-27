#include "ecm.h"
#include <utility>
#include <array>

namespace disc::format {
Ecm::Ecm(std::string file, std::vector<uint8_t> data) : file(std::move(file)), data(std::move(data)) {}

std::string Ecm::getFile() const { return file; }

disc::Position Ecm::getDiskSize() const { return disc::Position::fromLba(data.size() / Track::SECTOR_SIZE); }

size_t Ecm::getTrackCount() const { return 1; }

disc::Position Ecm::getTrackStart(int track) const { return disc::Position(0, 2, 0); }

disc::Position Ecm::getTrackLength(int track) const { return disc::Position::fromLba(data.size() / Track::SECTOR_SIZE); }

int Ecm::getTrackByPosition(disc::Position pos) const { return 1; }

disc::Sector Ecm::read(disc::Position pos) {
    std::vector<uint8_t> sector(Track::SECTOR_SIZE);

    size_t lba = (pos - disc::Position(0, 2, 0)).toLba() * Track::SECTOR_SIZE;
    if (lba + Track::SECTOR_SIZE >= data.size()) {
        return std::make_pair(sector, TrackType::INVALID);
    }

    std::copy(data.begin() + lba, data.begin() + lba + Track::SECTOR_SIZE, sector.begin());

    return std::make_pair(sector, TrackType::DATA);
}
}  // namespace disc::format