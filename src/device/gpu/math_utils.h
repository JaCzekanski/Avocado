#pragma once
#include <algorithm>
#include <glm/glm.hpp>

template <typename T>
T clamp(T number, size_t range) {
    if (number > range) number = range;
    return number;
}

template <typename T>
T clamp(T v, T min, T max) {
    return std::min(std::max(v, min), max);
}

inline float lerp(float a, float b, float t) { return a + t * (b - a); }

void precalcBarycentric(glm::ivec2 pos[3]);
glm::vec3 barycentric(glm::ivec2 pos[3], glm::ivec2 p);