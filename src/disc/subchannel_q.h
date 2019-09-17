#pragma once
#include <cstdint>
#include "position.h"
#include "utils/bcd.h"

namespace disc {
struct SubchannelQ {
    union Control {
        struct {
            uint8_t adr : 4;
            uint8_t audioPreemphasis : 1;
            uint8_t digitalCopy : 1;
            uint8_t data : 1;       // 0 - audio, 1 - data;
            uint8_t quadAudio : 1;  // 0 - stereo, 1 - quad
        };

        uint8_t reg;

        Control() : reg(0) {}
    };
    // Sync skipped
    Control control{};
    uint8_t data[9]{};
    uint16_t crc16{};

    static SubchannelQ generateForPosition(int track, disc::Position pos, disc::Position posInTrack, bool isAudio);
    uint16_t calculateCrc();
    bool validCrc();

    template <class Archive>
    void serialize(Archive& ar) {
        ar(control.reg, data, crc16);
    }
};
};  // namespace disc