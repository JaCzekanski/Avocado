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

/**
 * Sign extend integer to bigger size.
 * eg. 11 bit int to int16_t
 * THPS games does not use upper bits resulting in invalid coords.
 *
 * bit_size - number of bits INCLUDING sign bit
 */
template <size_t bit_size, typename T = int16_t>
T extend_sign(uint64_t n) {
    static_assert(bit_size > 0 && bit_size < 63, "bit_size out of range");

    T mask = ((1LL << (bit_size - 1)) - 1);
    bool sign = (n & (1LL << (bit_size - 1))) != 0;

    T val = n & mask;
    if (sign) val |= ~mask;
    return val;
}