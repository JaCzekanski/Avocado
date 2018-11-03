#include <position.h>
#include <cstdint>
#include <vector>

namespace disk {
typedef std::vector<uint8_t> Data;
typedef std::vector<uint8_t> Subcode;
typedef std::pair<Data, Subcode> Sector;

class Disk {
    virtual Sector read(uint32_t lba) = 0;

    virtual size_t getTrackCount() const = 0;
    virtual utils::Position getTrackLength(int track) const = 0;
};
}  // namespace disk