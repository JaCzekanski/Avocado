#pragma once
#include <vector>
#include "device/device.h"
#include "adsr.h"
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

        bool loopEnd;

        // ADPCM decoding
        int32_t prevSample[2];
        std::vector<int16_t> decodedSamples;

        Voice() {
            volume._reg = 0;
            sampleRate._reg = 0;
            startAddress._reg = 0;
            ADSR._reg = 0;
            ADSRVolume._reg = 0;
            repeatAddress._reg = 0;
            currentAddress._reg = 0;
            subAddress = 0;
            state = State::Off;
            loopEnd = false;
            mode = Mode::ADSR;
            pitchModulation = false;
            reverb = false;

            prevSample[0] = prevSample[1] = 0;
        }
    };
}