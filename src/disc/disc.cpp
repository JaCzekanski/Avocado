#include "disc.h"
#include <fmt/core.h>
#include "utils/file.h"

namespace disc {
SubchannelQ Disc::getSubQ(Position pos) {
    if (modifiedQ.find(pos) != modifiedQ.end()) {
        return modifiedQ[pos];
    }

    TrackType type = read(pos).second;
    int track = getTrackByPosition(pos);
    auto posInTrack = pos - getTrackStart(track);

    return SubchannelQ::generateForPosition(track, pos, posInTrack, type == TrackType::AUDIO);
}

bool Disc::loadSubchannel(const std::string& path) {
    std::string basePath = getPath(path) + getFilename(path);

    if (auto f = getFileContents(basePath + ".lsd"); !f.empty()) {
        return loadLsd(f);
    }
    if (auto f = getFileContents(basePath + ".sbi"); !f.empty()) {
        return loadSbi(f);
    }

    return false;
}

bool Disc::loadLsd(const std::vector<uint8_t>& lsd) {
    if (lsd.empty()) {
        fmt::print("[DISC] LSD file is empty\n");
        return false;
    }

    const int bytesPerEntry = 15;
    const int count = lsd.size() / bytesPerEntry;

    for (int i = 0; i < count; i++) {
        int p = bytesPerEntry * i;

        int mm = bcd::toBinary(lsd[p + 0]);
        int ss = bcd::toBinary(lsd[p + 1]);
        int ff = bcd::toBinary(lsd[p + 2]);

        Position pos(mm, ss, ff);

        SubchannelQ q;
        q.control._ = lsd[p + 3];
        for (int j = 0; j < 9; j++) {
            q.data[j] = lsd[p + 4 + j];
        }
        q.crc16 = (lsd[p + 12] << 8) | lsd[p + 13];

        modifiedQ[pos] = q;
    }

    fmt::print("[DISC] Loaded LSD file\n");
    return true;
}

bool Disc::loadSbi(const std::vector<uint8_t>& sbi) {
    if (sbi.empty()) {
        fmt::print("[DISC] SBI file is empty\n");
        return false;
    }

    if (sbi[0] != 'S' || sbi[1] != 'B' || sbi[2] != 'I' || sbi[3] != '\0') {
        fmt::print("[DISC] Invalid sbi header\n");
        return false;
    }

    const int headerSize = 4;
    const int bytesPerEntry = 14;
    const int count = (sbi.size() - headerSize) / bytesPerEntry;

    for (int i = 0; i < count; i++) {
        int p = headerSize + bytesPerEntry * i;

        int mm = bcd::toBinary(sbi[p + 0]);
        int ss = bcd::toBinary(sbi[p + 1]);
        int ff = bcd::toBinary(sbi[p + 2]);
        int dummy = sbi[p + 3];
        if (dummy != 1) {
            fmt::print("[DISC] Unsupported .sbi file, please create an issue on Github and attach this .sbi\n");
            return false;
        }

        Position pos(mm, ss, ff);

        SubchannelQ q;
        q.control._ = sbi[p + 4];
        for (int j = 0; j < 9; j++) {
            q.data[j] = sbi[p + 5 + j];
        }
        // Sbi does not include CRC-16, LibCrypt checks only if crc is broken
        q.crc16 = ~q.calculateCrc();

        modifiedQ[pos] = q;
    }

    fmt::print("[DISC] Loaded SBI file\n");
    return true;
}
}  // namespace disc