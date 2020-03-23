#include "voice.h"
#include <algorithm>
#include <cmath>
#include "sound/adpcm.h"
#include "utils/math.h"

namespace spu {
Voice::Voice() {
    volume._reg = 0;
    sampleRate._reg = 0;
    startAddress._reg = 0;
    adsr._reg = 0;
    adsrVolume._reg = 0;
    repeatAddress._reg = 0;
    currentAddress._reg = 0;
    ignoreLoadRepeatAddress = false;
    counter._reg = 0;
    state = State::Off;
    loopEnd = false;
    mode = Mode::ADSR;
    pitchModulation = false;
    reverb = false;
    adsrWaitCycles = 0;
    loadRepeatAddress = false;

    prevSample[0] = prevSample[1] = 0;

    enabled = true;
}

Envelope Voice::getCurrentPhase() {
    switch (state) {
        case State::Attack: return adsr.attack();
        case State::Decay: return adsr.decay();
        case State::Sustain: return adsr.sustain();
        case State::Release:
        default: return adsr.release();
    }
}

Voice::State Voice::nextState(Voice::State current) {
    switch (current) {
        case State::Attack: return State::Decay;
        case State::Decay: return State::Sustain;
        case State::Sustain: return State::Release;
        case State::Release:
        default: return State::Off;
    }
}

void Voice::processEnvelope() {
    using Mode = Envelope::Mode;
    using Dir = Envelope::Direction;
    if (state == State::Off) return;

    Envelope e = getCurrentPhase();

    if (adsrWaitCycles > 0) adsrWaitCycles--;

    auto cycles = 1 << std::max(0, e.shift - 11);
    int step = e.getStep() << std::max(0, 11 - e.shift);

    if (e.mode == Mode::Exponential) {
        if (e.direction == Dir::Increase && adsrVolume._reg > 0x6000) {
            cycles *= 4;
        }
        if (e.direction == Dir::Decrease) {
            // Note: Division by 0x8000 might cause value to become 0,
            // using right shift by 15 ensures that minimum it can go is -1.
            // Games affected: Doom (when paused music does not fade out)
            // Little Princess - Marl Oukoku no Ningyou-hime 2 (voices stop playing after a while)
            step = (step * adsrVolume._reg) >> 15;
        }
    }

    if (adsrWaitCycles == 0) {
        adsrWaitCycles = cycles;
        adsrVolume._reg = clamp(static_cast<int32_t>(adsrVolume._reg) + step, 0, 0x7fff);

        if (e.level != -1
            && ((e.direction == Dir::Increase && adsrVolume._reg >= e.level)
                || (e.direction == Dir::Decrease && adsrVolume._reg <= e.level))) {
            state = nextState(state);
            adsrWaitCycles = 0;
        }
    }
}

void Voice::parseFlags(uint8_t flags) {
    flagsParsed = true;

    // Mark beginning of the loop
    if (flags & ADPCM::Flag::LoopStart) {
        repeatAddress = currentAddress;
    }

    // Jump to Repeat address AFTER playing current sample
    if (flags & ADPCM::Flag::LoopEnd) {
        loopEnd = true;
        loadRepeatAddress = true;

        // if Repeat == 0 - force Release
        if (!(flags & ADPCM::Flag::Repeat) && mode != Mode::Noise) {
            adsrVolume._reg = 0;
            keyOff();
        }
    }
}

void Voice::keyOn(uint64_t cycles) {
    decodedSamples.clear();
    counter.sample = 0;
    adsrVolume._reg = 0;

    // INFO: Square games load repeatAddress before keyOn,
    // setting it here to startAddress causes glitched sample repetitions.
    // ignoreLoadRepeatAddress is set to true on write to repeatAddress register
    if (!ignoreLoadRepeatAddress) {
        repeatAddress._reg = startAddress._reg;
        ignoreLoadRepeatAddress = true;
    }

    currentAddress._reg = startAddress._reg;
    state = Voice::State::Attack;
    loopEnd = false;
    loadRepeatAddress = false;
    adsrWaitCycles = 0;

    prevSample[0] = prevSample[1] = 0;
    prevDecodedSamples.clear();

    this->cycles = cycles;
}

void Voice::keyOff(uint64_t cycles) {
    // SPU seems to ignore KeyOff events that were fired close to KeyOn.
    // Value of 384 cycles was picked by listening to Dragon Ball Final Bout - BGM 29
    // and comparing it to recording from real HW.
    // It's not verified by any tests for now
    if (cycles != 0 && cycles - this->cycles < 384) {
        return;
    }
    state = Voice::State::Release;
    adsrWaitCycles = 0;
}

}  // namespace spu