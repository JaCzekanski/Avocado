#pragma once

enum class ColorDepth { NONE, BIT_4, BIT_8, BIT_16 };

ColorDepth bitsToDepth(int bits);

template <int bits>
constexpr ColorDepth bitsToDepth() {
    if constexpr (bits == 4)
        return ColorDepth::BIT_4;
    else if constexpr (bits == 8)
        return ColorDepth::BIT_8;
    else if constexpr (bits == 16)
        return ColorDepth::BIT_16;
    else
        return ColorDepth::NONE;
}