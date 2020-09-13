#pragma once
#include <array>
#include <cstdint>
namespace gte {
using Matrix = std::array<std::array<int16_t, 3>, 3>;

template <typename X, typename Y = X, typename Z = X>
struct Vector {
    union {
        X x, r;
    };

    union {
        Y y, g;
    };

    union {
        Z z, b;
    };

    Vector() = default;
    Vector(X x) : x(x), y(x), z(x) {}
    Vector(X x, Y y, Z z) : x(x), y(y), z(z) {}

    template <class Archive>
    void serialize(Archive &ar) {
        ar(x, y, z);
    }
};

Vector<int16_t> toVector(int16_t ir[4]);

struct Color {
    int32_t r = 0;
    int32_t g = 0;
    int32_t b = 0;
};

int isin(int x);

int icos(int x);

gte::Matrix getIdentity();

gte::Matrix mulMatrix(gte::Matrix &matrixA, gte::Matrix &matrixB);

gte::Matrix invMatrix(gte::Matrix &matrix);

gte::Matrix rotMatrix(gte::Vector<int16_t> angles);

gte::Matrix rotMatrixX(int16_t angle);

gte::Matrix rotMatrixY(int16_t angle);

gte::Matrix rotMatrixZ(int16_t angle);

gte::Vector<int32_t> applyMatrix(gte::Matrix &matrix, gte::Vector<int32_t> &vector);

gte::Vector<int32_t> transVector(gte::Vector<int32_t> &vector, int32_t x, int32_t y, int32_t z);

gte::Vector<int32_t> normalizeVector(gte::Vector<int32_t> &vector);

gte::Vector<int32_t> crossVector(gte::Vector<int32_t> &vectorA, gte::Vector<int32_t> &vectorB);

};  // namespace gte