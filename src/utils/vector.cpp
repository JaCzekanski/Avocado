#include "vector.h"
#include <cmath>

vec2::vec2(const ivec2& v) : x(static_cast<float>(v.x)), y(static_cast<float>(v.y)) {}

ivec3::ivec3(const vec3& v) : x(static_cast<int>(v.x)), y(static_cast<int>(v.y)), z(static_cast<int>(v.z)) {}

float vec2::length() const { return sqrtf(x * x + y * y); }

vec2 vec2::normalize(const vec2& v) {
    float length = v.length();
    return {v.x / length, v.y / length};
}

vec3::vec3(const ivec3& v) : x(static_cast<float>(v.x)), y(static_cast<float>(v.y)), z(static_cast<float>(v.z)) {}

float vec3::length() const { return sqrtf(x * x + y * y + z * z); }

vec3 vec3::normalize(const vec3& v) {
    float length = v.length();
    return {v.x / length, v.y / length, v.z / length};
}

vec3 vec3::cross(const vec3& a, const vec3& b) {
    return vec3(                //
        a.y * b.z - a.z * b.y,  //
        a.z * b.x - a.x * b.z,  //
        a.x * b.y - a.y * b.x   //
    );
}