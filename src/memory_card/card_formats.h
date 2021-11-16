#pragma once
#include <array>
#include <string>
#include <optional>
#include <vector>

namespace memory_card {
constexpr int MEMCARD_SIZE = 128 * 1024;
bool isMemoryCardImage(const std::string& path);
std::optional<std::vector<uint8_t>> load(const std::string& path);
bool save(const std::array<uint8_t, MEMCARD_SIZE>& data, const std::string& path);
void format(std::array<uint8_t, MEMCARD_SIZE>& data);
};  // namespace memory_card
