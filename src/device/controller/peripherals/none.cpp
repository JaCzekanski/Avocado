#include "none.h"

namespace peripherals {
None::None() : AbstractDevice(Type::None){
}

uint8_t None::handle(uint8_t byte) {
    return 0xff;
}
};  // namespace peripherals