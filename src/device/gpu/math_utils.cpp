#include "math_utils.h"

namespace {
glm::vec2 v0, v1;
float d00, d01, d11, denom;
};  // namespace

void precalcBarycentric(glm::ivec2 pos[3]) {
    v0 = pos[1] - pos[0];
    v1 = pos[2] - pos[0];
    d00 = glm::dot(v0, v0);
    d01 = glm::dot(v0, v1);
    d11 = glm::dot(v1, v1);
    denom = d00 * d11 - d01 * d01;
}

glm::vec3 barycentric(glm::ivec2 pos[3], glm::ivec2 p) {
    glm::vec2 v2 = p - pos[0];
    float d20 = glm::dot(v2, v0);
    float d21 = glm::dot(v2, v1);
    float v = (d11 * d20 - d01 * d21) / denom;
    float w = (d00 * d21 - d01 * d20) / denom;
    float u = 1.0f - v - w;

    return glm::vec3(u, v, w);
}