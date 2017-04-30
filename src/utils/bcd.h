#pragma once
#include <cstdint>

namespace bcd {
uint8_t toBinary(uint8_t bcd);
uint8_t toBcd(uint8_t binary);
}