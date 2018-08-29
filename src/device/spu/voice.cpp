#include "voice.h"
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
            step = static_cast<int>(static_cast<float>(step) * std::ceil(static_cast<float>(adsrVolume._reg) / static_cast<float>(0x8000)));
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
}  // namespace spu