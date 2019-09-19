#pragma once

template <typename T>
T clamp(T number, size_t range) {
    if (number > range) number = range;
    return number;
}

template <typename T>
T clamp(T v, T min, T max) {
    if (v > max) {
        return max;
    } else if (v < min) {
        return min;
    } else {
        return v;
    }
}

template <typename T>
T clamp_top(T v, T top) {
    return v > top ? top : v;
}

template <typename T>
T clamp_bottom(T v, T bottom) {
    return v < bottom ? bottom : v;
}

inline float lerp(float a, float b, float t) { return a + t * (b - a); }

// Convert normalized float to int16_t
inline int16_t floatToInt(float val) {
    if (val >= 0) {
        return static_cast<int16_t>(val * static_cast<int16_t>(INT16_MAX));
    } else {
        return static_cast<int16_t>(-val * static_cast<int16_t>(INT16_MIN));
    }
}

// Convert int16_t to normalized float
inline float intToFloat(int16_t val) {
    if (val >= 0) {
        return static_cast<float>(val) / static_cast<float>(INT16_MAX);
    } else {
        return -static_cast<float>(val) / static_cast<float>(INT16_MIN);
    }
}