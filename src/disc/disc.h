#pragma once
#include <cstdint>
#include <unordered_map>
#include <vector>
#include "position.h"
#include "subchannel_q.h"

namespace disc {
enum class TrackType { DATA, AUDIO, INVALID };
typedef std::vector<uint8_t> Data;
typedef std::vector<uint8_t> Subcode;
typedef std::pair<Data, TrackType> Sector;

struct Disc {
    virtual ~Disc() = default;
    virtual Sector read(Position pos) = 0;

    virtual std::string getFile() const = 0;
    virtual size_t getTrackCount() const = 0;
    virtual int getTrackByPosition(Position pos) const = 0;
    virtual Position getTrackStart(int track) const = 0;
    virtual Position getTrackLength(int track) const = 0;
    virtual Position getDiskSize() const = 0;

    SubchannelQ getSubQ(Position pos);
    bool loadSubchannel(const std::string& path);

   private:
    std::unordered_map<Position, SubchannelQ> modifiedQ;
    bool loadLsd(const std::vector<uint8_t>& lsd);
    bool loadSbi(const std::vector<uint8_t>& sbi);
};
}  // namespace disc