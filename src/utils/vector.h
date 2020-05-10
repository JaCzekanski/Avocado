#pragma once

struct ivec2 {
    int x, y;
    ivec2() = default;
    ivec2(int x, int y) : x(x), y(y) {}

    ivec2 operator-(const ivec2& b) const { return {x - b.x, y - b.y}; }
    ivec2 operator+(const ivec2& b) const { return {x + b.x, y + b.y}; }
    bool operator==(const ivec2& b) const { return x == b.x && y == b.y; }
    bool operator!=(const ivec2& b) const { return x != b.x || y != b.y; }
};

struct ivec3 {
    int x, y, z;
    ivec3(int x, int y, int z) : x(x), y(y), z(z) {}

    ivec3 operator-(const ivec3& b) const { return {x - b.x, y - b.y, z - b.z}; }
    ivec3 operator+(const ivec3& b) const { return {x + b.x, y + b.y, z - b.z}; }
    bool operator==(const ivec3& b) const { return x == b.x && y == b.y && z == b.z; }
    bool operator!=(const ivec3& b) const { return x != b.x || y != b.y || z != b.z; }
};

struct vec2 {
    float x, y;
    vec2() = default;
    vec2(float x, float y) : x(x), y(y) {}
    vec2(const ivec2& v) : x(static_cast<float>(v.x)), y(static_cast<float>(v.y)) {}

    vec2 operator-(const vec2& b) const { return {x - b.x, y - b.y}; }
    vec2 operator+(const vec2& b) const { return {x + b.x, y + b.y}; }
    vec2 operator*(const float b) const { return {x * b, y * b}; }
    vec2 operator/(const float b) const { return {x / b, y / b}; }
    bool operator==(const vec2& b) const { return x == b.x && y == b.y; }
    bool operator!=(const vec2& b) const { return x != b.x || y != b.y; }

    float length() const;
    static vec2 normalize(const vec2& v);
};
