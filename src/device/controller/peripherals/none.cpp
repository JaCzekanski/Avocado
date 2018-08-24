#include "none.h"

namespace peripherals {
None::None(int port) : AbstractDevice(Type::None, port) {}

uint8_t None::handle(uint8_t byte) { return 0xff; }
};  // namespace peripherals