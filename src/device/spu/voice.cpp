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
            step = (int)(ceilf((float)adsrVolume._reg / (float)0x8000) * (float)step);
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

    if (flags & ADPCM::Flag::Start) {
        repeatAddress = currentAddress;
    }

    if (flags & ADPCM::Flag::End) {
        // Jump to Repeat address AFTER playing current sample
        loopEnd = true;
        loadRepeatAddress = true;

        if (!(flags & ADPCM::Flag::Repeat) && mode != Mode::Noise) {
            adsrVolume._reg = 0;
            keyOff();
        }
    }
}

void Voice::keyOn() {
    decodedSamples.clear();
    counter.sample = 0;
    adsrVolume._reg = 0;

    // INFO: Square games load repeatAddress before keyOn,
    // setting it here to startAddress causes glitched sample repetitions.
    // ignoreLoadRepeatAddress is set to true on write to repeatAddress register
    if (!ignoreLoadRepeatAddress) {
        repeatAddress._reg = startAddress._reg;
        ignoreLoadRepeatAddress = false;
    }

    currentAddress._reg = startAddress._reg;
    state = Voice::State::Attack;
    loopEnd = false;
    loadRepeatAddress = false;
    adsrWaitCycles = 0;

    prevSample[0] = prevSample[1] = 0;
    prevDecodedSamples.clear();
}

void Voice::keyOff() {
    state = Voice::State::Release;
    adsrWaitCycles = 0;
}

}  // namespace spu