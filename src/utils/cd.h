#pragma once
#include <cstdint>

namespace cd {
union Submode {
    struct {
        uint8_t endOfRecord : 1;
        uint8_t video : 1;
        uint8_t audio : 1;
        uint8_t data : 1;
        uint8_t trigger : 1;  // unused
        uint8_t form2 : 1;    // Mode2 Form1/2
        uint8_t realtime : 1;
        uint8_t endOfFile : 1;
    };
    uint8_t _byte;

    Submode(uint8_t b) : _byte(b) {}
};

union Codinginfo {
    struct {
        uint8_t stereo : 1;  // 0 - mono, 1 - stereo
        uint8_t : 1;
        uint8_t sampleRate : 1;  // 0 - 37800Hz, 1 - 18900Hz
        uint8_t : 1;
        uint8_t bits : 1;  // 0 - 4bit, 1 - 8bit
        uint8_t : 1;
        uint8_t emphasis : 1;
        uint8_t : 1;
    };
    uint8_t _byte;

    Codinginfo(uint8_t b) : _byte(b) {}
};
}  // namespace cd