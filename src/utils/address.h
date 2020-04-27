#pragma once
#include <cstdint>
#include <type_traits>
#include "macros.h"

template <typename T>
INLINE constexpr uint32_t align_mips(uint32_t address) {
    if constexpr (sizeof(T) == 1)
        return address & 0x1fffffff;
    else if constexpr (sizeof(T) == 2)
        return address & 0x1ffffffe;
    else if constexpr (sizeof(T) == 4)
        return address & 0x1ffffffc;
    else
        static_assert(std::is_same<T, uint8_t>() || std::is_same<T, uint16_t>() || std::is_same<T, uint32_t>(), "Invalid type used");
}

template <uint32_t base, uint32_t size>
INLINE constexpr bool in_range(const uint32_t addr) {
    return (addr >= base && addr < base + size);
}