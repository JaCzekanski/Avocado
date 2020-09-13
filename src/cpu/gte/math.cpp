#include <cmath>
#include "math.h"

#define ONE 4096
#define qN 10
#define qA 12
#define B 19900
#define C 3516

namespace gte {
Vector<int16_t> toVector(int16_t ir[4]) { return Vector<int16_t>(ir[1], ir[2], ir[3]); }

int isin(int x) {
    int c, x2, y;

    c = x << (30 - qN);  // Semi-circle info into carry.
    x -= 1 << qN;        // sine -> cosine calc

    x = x << (31 - qN);  // Mask with PI
    x = x >> (31 - qN);  // Note: SIGNED shift! (to qN)

    x = x * x >> (2 * qN - 14);  // x=x^2 To Q14

    y = B - (x * C >> 14);          // B - x^2*C
    y = (1 << qA) - (x * y >> 16);  // A - x^2*(B-x^2*C)

    return c >= 0 ? y : -y;
}

int icos(int x) { return isin(x + 1024); }

gte::Matrix getIdentity() {
    gte::Matrix result;
    result[0][0] = ONE;
    result[0][1] = 0;
    result[0][2] = 0;
    result[1][0] = 0;
    result[1][1] = ONE;
    result[1][2] = 0;
    result[2][0] = 0;
    result[2][1] = 0;
    result[2][2] = ONE;
    return result;
}

gte::Matrix mulMatrix(gte::Matrix &matrixA, gte::Matrix &matrixB) {
    gte::Matrix result;
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            result[i][j] = 0;
        }
    }
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            for (int k = 0; k < 3; k++) {
                result[i][j] += ((matrixB[k][j] * matrixA[i][k]) >> 12);
            }
        }
    }
    return result;
}

gte::Matrix invMatrix(gte::Matrix &matrix) {
    gte::Matrix result;
    int16_t determinant = 0;
    for (int i = 0; i < 3; i++) {
        determinant
            = determinant
              + (matrix[0][i] * (matrix[1][(i + 1) % 3] * matrix[2][(i + 2) % 3] - matrix[1][(i + 2) % 3] * matrix[2][(i + 1) % 3]));
    }
    if (determinant == 0) {
        return matrix;
    }
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            result[i][j] = ((matrix[(j + 1) % 3][(i + 1) % 3] * matrix[(j + 2) % 3][(i + 2) % 3])
                            - (matrix[(j + 1) % 3][(i + 2) % 3] * matrix[(j + 2) % 3][(i + 1) % 3]))
                           / determinant;
        }
    }
    return result;
}

gte::Matrix rotMatrix(gte::Vector<int16_t> angles) {
    gte::Matrix result;

    int16_t s[3], c[3];
    gte::Matrix temp[3];

    s[0] = isin(angles.x);
    s[1] = isin(angles.y);
    s[2] = isin(angles.z);
    c[0] = icos(angles.x);
    c[1] = icos(angles.y);
    c[2] = icos(angles.z);

    // mX
    result[0][0] = ONE;
    result[0][1] = 0;
    result[0][2] = 0;
    result[1][0] = 0;
    result[1][1] = c[0];
    result[1][2] = -s[0];
    result[2][0] = 0;
    result[2][1] = s[0];
    result[2][2] = c[0];
    // mY
    temp[0][0][0] = c[1];
    temp[0][0][1] = 0;
    temp[0][0][2] = s[1];
    temp[0][1][0] = 0;
    temp[0][1][1] = ONE;
    temp[0][1][2] = 0;
    temp[0][2][0] = -s[1];
    temp[0][2][1] = 0;
    temp[0][2][2] = c[1];
    // mZ
    temp[1][0][0] = c[2];
    temp[1][0][1] = -s[2];
    temp[1][0][2] = 0;
    temp[1][1][0] = s[2];
    temp[1][1][1] = c[2];
    temp[1][1][2] = 0;
    temp[1][2][0] = 0;
    temp[1][2][1] = 0;
    temp[1][2][2] = ONE;

    temp[2] = mulMatrix(result, temp[0]);
    result = mulMatrix(temp[2], temp[1]);

    return result;
}

gte::Matrix rotMatrixX(int16_t angle) {
    gte::Matrix result;
    int s = isin(angle);
    int c = icos(angle);
    result[0][0] = ONE;
    result[0][1] = 0;
    result[0][2] = 0;
    result[1][0] = 0;
    result[1][1] = c;
    result[1][2] = -s;
    result[2][0] = 0;
    result[2][1] = s;
    result[2][2] = c;
    return result;
}

gte::Matrix rotMatrixY(int16_t angle) {
    gte::Matrix result;
    int s = isin(angle);
    int c = icos(angle);
    result[0][0] = c;
    result[0][1] = 0;
    result[0][2] = s;
    result[1][0] = 0;
    result[1][1] = ONE;
    result[1][2] = 0;
    result[2][0] = -s;
    result[2][1] = 0;
    result[2][2] = c;
    return result;
}

gte::Matrix rotMatrixZ(int16_t angle) {
    gte::Matrix result;
    int s = isin(angle);
    int c = icos(angle);
    result[0][0] = c;
    result[0][1] = -s;
    result[0][2] = 0;
    result[1][0] = s;
    result[1][1] = c;
    result[1][2] = 0;
    result[2][0] = 0;
    result[2][1] = 0;
    result[2][2] = ONE;
    return result;
}

gte::Vector<int32_t> applyMatrix(gte::Matrix &matrix, gte::Vector<int32_t> &vector) {
    gte::Vector<int32_t> result;
    result.x = (((matrix[0][0] * vector.x) + (matrix[0][1] * vector.y) + (matrix[0][2] * vector.z)) >> 12);
    result.y = (((matrix[1][0] * vector.x) + (matrix[1][1] * vector.y) + (matrix[1][2] * vector.z)) >> 12);
    result.z = (((matrix[2][0] * vector.x) + (matrix[2][1] * vector.y) + (matrix[2][2] * vector.z)) >> 12);
    return result;
}

gte::Vector<int32_t> transVector(gte::Vector<int32_t> &vector, int32_t x, int32_t y, int32_t z) {
    gte::Vector<int32_t> result;
    result.x = vector.x + x;
    result.y = vector.y + y;
    result.z = vector.z + z;
    return result;
}

gte::Vector<int32_t> normalizeVector(gte::Vector<int32_t> &vector) {
    gte::Vector<int32_t> result;
    float squareRoot = std::sqrt((vector.x * vector.x) + (vector.y * vector.y) + (vector.z * vector.z));
    result.x = vector.x / squareRoot;
    result.y = vector.y / squareRoot;
    result.z = vector.z / squareRoot;
    return result;
}

gte::Vector<int32_t> crossVector(gte::Vector<int32_t> &vectorA, gte::Vector<int32_t> &vectorB) {
    gte::Vector<int32_t> result;
    result.x = vectorA.y * vectorB.z - vectorA.z * vectorB.y;
    result.y = vectorA.z * vectorB.x - vectorA.x * vectorB.z;
    result.z = vectorA.x * vectorB.y - vectorA.y * vectorB.x;
    return result;
}

};  // namespace gte