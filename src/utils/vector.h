#pragma once
#include <cstddef>

struct ivec2;
struct ivec3;
struct vec2;
struct vec3;

struct ivec2 {
    int x = 0, y = 0;
    ivec2() = default;
    ivec2(int x, int y) : x(x), y(y) {}

    ivec2 operator-(const ivec2& b) const { return {x - b.x, y - b.y}; }
    ivec2 operator+(const ivec2& b) const { return {x + b.x, y + b.y}; }
    bool operator==(const ivec2& b) const { return x == b.x && y == b.y; }
    bool operator!=(const ivec2& b) const { return x != b.x || y != b.y; }
};

struct ivec3 {
    int x = 0, y = 0, z = 0;
    ivec3(int x, int y, int z) : x(x), y(y), z(z) {}
    ivec3(const vec3& v);

    ivec3 operator-(const ivec3& b) const { return {x - b.x, y - b.y, z - b.z}; }
    ivec3 operator+(const ivec3& b) const { return {x + b.x, y + b.y, z - b.z}; }
    bool operator==(const ivec3& b) const { return x == b.x && y == b.y && z == b.z; }
    bool operator!=(const ivec3& b) const { return x != b.x || y != b.y || z != b.z; }
};

struct vec2 {
    float x = 0.f, y = 0.f;
    vec2() = default;
    vec2(float x, float y) : x(x), y(y) {}
    vec2(const ivec2& v);

    vec2 operator-(const vec2& b) const { return {x - b.x, y - b.y}; }
    vec2 operator+(const vec2& b) const { return {x + b.x, y + b.y}; }
    vec2 operator*(const float b) const { return {x * b, y * b}; }
    vec2 operator/(const float b) const { return {x / b, y / b}; }
    bool operator==(const vec2& b) const { return x == b.x && y == b.y; }
    bool operator!=(const vec2& b) const { return x != b.x || y != b.y; }

    float length() const;
    static vec2 normalize(const vec2& v);
};

struct vec3 {
    float x = 0.f, y = 0.f, z = 0.f;
    vec3() = default;
    vec3(float x, float y, float z) : x(x), y(y), z(z) {}
    vec3(const ivec3& v);

    vec3 operator-(const vec3& b) const { return {x - b.x, y - b.y, z - b.z}; }
    vec3 operator+(const vec3& b) const { return {x + b.x, y + b.y, z - b.z}; }
    vec3 operator*(const float b) const { return {x * b, y * b, z * b}; }
    vec3 operator/(const float b) const { return {x / b, y / b, z / b}; }
    bool operator==(const vec3& b) const { return x == b.x && y == b.y && z == b.z; }
    bool operator!=(const vec3& b) const { return x != b.x || y != b.y || z != b.z; }

    float length() const;
    static vec3 normalize(const vec3& v);
    static vec3 cross(const vec3& a, const vec3& b);
};
