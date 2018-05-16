#pragma once
#include <algorithm>
#include <cstdint>

template <size_t from, size_t to>
bool or_range(uint32_t v) {
    static_assert(from >= 0 && from < 32, "from out of range");
    static_assert(to >= 0 && to < 32, "to out of range");

    const size_t max = std::max(from, to);
    const size_t min = std::min(from, to);

    bool result = false;
    for (size_t i = min; i <= max; ++i) {
        result |= (v & (1 << i)) != 0;
    }
    return result;
}