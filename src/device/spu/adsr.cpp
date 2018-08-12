#include "adsr.h"

namespace spu {

int Envelope::getStep() const {
    if (direction == Direction::Increase) {
        return 4 + step;
    } else {
        return -step - 5;
    }
}

ADSR::ADSR() : _reg(0) {}

void ADSR::write(int n, uint8_t v) {
    if (n >= 4) return;
    _byte[n] = v;
}

uint8_t ADSR::read(int n) const {
    if (n >= 4) return 0;
    return _byte[n];
}

Envelope ADSR::attack() {
    Envelope e;
    e.level = 0x7fff;
    e.step = attackStep;
    e.shift = attackShift;
    e.direction = Envelope::Direction::Increase;
    e.mode = static_cast<Envelope::Mode>(attackMode);
    return e;
}

Envelope ADSR::decay() {
    Envelope e;
    e.level = (sustainLevel + 1) * 0x800;
    e.step = 3;  // -8
    e.shift = decayShift;
    e.direction = Envelope::Direction::Decrease;
    e.mode = Envelope::Mode::Exponential;
    return e;
}

Envelope ADSR::sustain() {
    Envelope e;
    e.level = -1;  // sustain phase never ends
    e.step = sustainStep;
    e.shift = sustainShift;
    e.direction = static_cast<Envelope::Direction>(sustainDirection);
    e.mode = static_cast<Envelope::Mode>(sustainMode);
    return e;
}

Envelope ADSR::release() {
    Envelope e;
    e.level = 0;
    e.step = 3;  // -8
    e.shift = releaseShift;
    e.direction = Envelope::Direction::Decrease;
    e.mode = static_cast<Envelope::Mode>(releaseMode);
    return e;
}
}  // namespace spu