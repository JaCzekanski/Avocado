#pragma once
#include <vector>
#include "device/device.h"
#include "adsr.h"
#include "regs.h"
#include "utils/math.h"

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
            adsrWaitCycles = 0;
            loadRepeatAddress = false;

            prevSample[0] = prevSample[1] = 0;
        }

        Envelope getCurrentPhase() {
            if (state == State::Attack) return ADSR.attack();
            else if (state == State::Decay) return ADSR.decay();
            else if (state == State::Sustain) return ADSR.sustain();
            else return ADSR.release();
        }

        State nextState(State current) {
            switch (current) {
                case State::Attack: return State::Decay;
                case State::Decay: return State::Sustain;
                case State::Sustain: return State::Release;
                case State::Release: return State::Off;
                default: return State::Off;
            }
        }

        void processEnvelope() {
            using Mode = Envelope::Mode;
            using Dir = Envelope::Direction;
            if (state == State::Off) return;

            Envelope e = getCurrentPhase();

            if (adsrWaitCycles > 0) adsrWaitCycles--;

            auto cycles = 1 << std::max(0, e.shift - 11);
            int step = e.getStep() << std::max(0, 11 - e.shift);

            if (e.mode == Mode::Exponential) {
                if (e.direction == Dir::Increase && ADSRVolume._reg > 0x6000) {
                    cycles *= 4;
                }
                if (e.direction == Dir::Decrease) {
                    step = static_cast<float>(step) * static_cast<float>(ADSRVolume._reg) / static_cast<float>(0x8000);
                }
            }

            // Wait cycles
            if (adsrWaitCycles == 0) {
                adsrWaitCycles = cycles;
                ADSRVolume._reg = clamp(static_cast<int32_t>(ADSRVolume._reg) + step, 0, 0x7fff);

                if (e.level != -1 &&
                    ((e.direction == Dir::Increase && ADSRVolume._reg >= e.level) ||
                    (e.direction == Dir::Decrease && ADSRVolume._reg <= e.level))) {
                    
                    state = nextState(state);
                    adsrWaitCycles = 0;
                }
            }
        }
    };
}