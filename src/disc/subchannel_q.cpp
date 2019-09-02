#include "subchannel_q.h"

namespace disc {

SubchannelQ SubchannelQ::generateForPosition(int track, disc::Position pos, disc::Position posInTrack, bool isAudio) {
    SubchannelQ q;
    q.control.adr = 1;
    q.control.data = !isAudio;

    q.data[0] = bcd::toBcd(track + 1);      // Track
    q.data[1] = bcd::toBcd(1);              // Index
    q.data[2] = bcd::toBcd(posInTrack.mm);  // M - Track
    q.data[3] = bcd::toBcd(posInTrack.ss);  // S - Track
    q.data[4] = bcd::toBcd(posInTrack.ff);  // F - Track
    q.data[5] = 0;                          // Reserved
    q.data[6] = bcd::toBcd(pos.mm);         // M - Disc
    q.data[7] = bcd::toBcd(pos.ss);         // S - Disc
    q.data[8] = bcd::toBcd(pos.ff);         // F - Disc
    q.crc16 = q.calculateCrc();

    return q;
}

uint16_t SubchannelQ::calculateCrc() {
    uint8_t lsb = 0;
    uint8_t msb = 0;
    for (int i = 0; i < 0x0a; i++) {
        uint8_t x;
        if (i == 0) {
            x = control._;
        } else {
            x = data[i - 1];
        }
        x ^= msb;
        x = x ^ (x >> 4);
        msb = lsb ^ (x >> 3) ^ (x << 4);
        lsb = x ^ (x << 5);
    }
    return msb << 8 | lsb;
}

bool SubchannelQ::validCrc() { return crc16 == calculateCrc(); }
};  // namespace disc