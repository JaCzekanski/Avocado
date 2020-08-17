#pragma once
#include <cstdint>
#include <vector>

namespace wave {
bool writeToFile(const std::vector<uint16_t>& buffer, const char* filename, int channels = 2);
};
