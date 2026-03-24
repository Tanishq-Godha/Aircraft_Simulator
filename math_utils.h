#ifndef MATH_UTILS_H
#define MATH_UTILS_H

#include <cmath>

const float PI = 3.14159265f;
const float GRAVITY = 9.81f;

inline float degToRad(float d) { return d * PI / 180.0f; }

inline float lerp(float a, float b, float t) {
    return a + t * (b - a);
}

#endif