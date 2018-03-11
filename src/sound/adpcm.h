#pragma once
#include <vector>

namespace ADPCM {
std::vector<int16_t> decode(std::vector<uint8_t> buffer);
};  // namespace ADPCM