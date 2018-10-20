#pragma once
#include <vector>
#include "adsr.h"
#include "device/device.h"
#include "regs.h"

namespace spu {
struct Voice {
    enum class State { Attack, Decay, Sustain, Release, Off };
    enum class Mode { ADSR, Noise };
    union Counter {
        struct {
            uint32_t : 4;
            uint32_t index : 8;  // Interpolation index
            uint32_t sample : 5;
            uint32_t : 15;
        };
        uint32_t _reg = 0;
    };
    Volume volume;
    Reg16 sampleRate;
    Reg16 startAddress;
    ADSR adsr;
    Reg16 adsrVolume;
    Reg16 repeatAddress;

    Reg16 currentAddress;
    Counter counter;
    State state;
    Mode mode;
    bool pitchModulation;
    bool reverb;

    int adsrWaitCycles;
    bool loopEnd;
    bool loadRepeatAddress;
    bool flagsParsed;

    // ADPCM decoding
    int32_t prevSample[2];
    std::vector<int16_t> decodedSamples;
    std::vector<int16_t> prevDecodedSamples;

    Voice();
    Envelope getCurrentPhase();
    State nextState(State current);
    void processEnvelope();
    void parseFlags(uint8_t flags);

    void keyOn();
    void keyOff();
};
}  // namespace spu
