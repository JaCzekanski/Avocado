#pragma once
#include <vector>
#include "adsr.h"
#include "device/device.h"
#include "regs.h"

namespace spu {
struct Voice {
    enum class State { Attack, Decay, Sustain, Release, Off };
    enum class Mode { ADSR, Noise };
    Volume volume;
    Reg16 sampleRate;
    Reg16 startAddress;
    ADSR ADSR;
    Reg16 ADSRVolume;
    Reg16 repeatAddress;

    Reg16 currentAddress;
    float subAddress;
    State state;
    Mode mode;
    bool pitchModulation;
    bool reverb;

    int adsrWaitCycles;
    bool loopEnd;
    bool loadRepeatAddress;

    // ADPCM decoding
    int32_t prevSample[2];
    std::vector<int16_t> decodedSamples;

    Voice();
    Envelope getCurrentPhase();
    State nextState(State current);
    void processEnvelope();
};
}  // namespace spu