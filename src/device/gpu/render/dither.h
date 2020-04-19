#pragma once
#include <cstdint>
#include <array>

using dither_lut_t = std::array<std::array<std::array<uint8_t, 256 * 4 * 4>, 4>, 4>;
extern const dither_lut_t ditherLUT;