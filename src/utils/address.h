#pragma once
#include <cstdint>
#include <type_traits>

template <typename T>
constexpr uint32_t align_mips(uint32_t address) {
    static_assert(std::is_same<T, uint8_t>() || std::is_same<T, uint16_t>() || std::is_same<T, uint32_t>(), "Invalid type used");

    if (sizeof(T) == 1) return address & 0x1fffffff;
    if (sizeof(T) == 2) return address & 0x1ffffffe;
    if (sizeof(T) == 4) return address & 0x1ffffffc;
    return 0;
}

template <uint32_t base, uint32_t size>
constexpr bool in_range(const uint32_t addr) {
    return (addr >= base && addr < base + size);
}