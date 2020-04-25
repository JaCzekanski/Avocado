#pragma once
#include <cstdint>

namespace timing {
constexpr uint64_t US_IN_SECOND = 1'000'000;
constexpr uint64_t CPU_CLOCK = 33'868'800;  // 44100 * 768

constexpr uint64_t usToCpuCycles(uint64_t us) { return us * CPU_CLOCK / US_IN_SECOND; }
}  // namespace timing
