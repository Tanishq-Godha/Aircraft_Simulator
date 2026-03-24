#include "terrain.h"
#include "globals.h"
#include "math_utils.h"
#include <GL/glut.h>
#include <cmath>

float getVoxelHeight(float x, float z);

namespace {

void drawVoxelColumn(float x, float z, float cellSize) {
    float h = getVoxelHeight(x + cellSize * 0.5f, z + cellSize * 0.5f);

    if (h <= BLOCK_SIZE * 2) glColor3f(0.1f, 0.35f, 0.85f);
    else if (h <= BLOCK_SIZE * 3) glColor3f(0.85f, 0.85f, 0.6f);
    else if (h <= BLOCK_SIZE * 7) glColor3f(0.3f, 0.7f, 0.25f);
    else if (h <= BLOCK_SIZE * 11) glColor3f(0.55f, 0.55f, 0.55f);
    else glColor3f(0.95f, 0.95f, 0.95f);

    glPushMatrix();
    glTranslatef(x + cellSize * 0.5f, h * 0.5f, z + cellSize * 0.5f);
    glScalef(cellSize, h, cellSize);
    glutSolidCube(1.0f);
    glPopMatrix();
}

}

float hash(float x, float y) {
    float h = sin(x * 12.9898f + y * 78.233f) * 43758.5453123f;
    return h - floor(h);
}

float noise(float x, float y) {
    float ix = floor(x), iy = floor(y);
    float fx = x - ix, fy = y - iy;

    float ux = fx * fx * (3 - 2 * fx);
    float uy = fy * fy * (3 - 2 * fy);

    float a = hash(ix, iy);
    float b = hash(ix + 1, iy);
    float c = hash(ix, iy + 1);
    float d = hash(ix + 1, iy + 1);

    return lerp(lerp(a, b, ux), lerp(c, d, ux), uy);
}

float getRawHeight(float x, float z) {
    float total = 0, freq = 0.0002f, amp = 1800, pers = 0.45f;

    for (int i = 0; i < 4; i++) {
        total += noise(x * freq, z * freq) * amp;
        freq *= 2;
        amp *= pers;
    }
    return total;
}

float getVoxelHeight(float x, float z) {
    float raw = getRawHeight(x, z);
    float snapped = floor(raw / BLOCK_SIZE) * BLOCK_SIZE;
    return snapped < BLOCK_SIZE ? BLOCK_SIZE : snapped;
}

void drawVoxelTerrain() {
    float nearRadius = 3000.0f;
    float farRadius = 7800.0f;
    float nearStep = BLOCK_SIZE;
    float farStep = BLOCK_SIZE * 3.0f;

    float nearStartX = floor((planeX - nearRadius) / nearStep) * nearStep;
    float nearStartZ = floor((planeZ - nearRadius) / nearStep) * nearStep;

    for (float x = nearStartX; x <= planeX + nearRadius; x += nearStep) {
        for (float z = nearStartZ; z <= planeZ + nearRadius; z += nearStep) {
            drawVoxelColumn(x, z, nearStep);
        }
    }

    float farStartX = floor((planeX - farRadius) / farStep) * farStep;
    float farStartZ = floor((planeZ - farRadius) / farStep) * farStep;

    for (float x = farStartX; x <= planeX + farRadius; x += farStep) {
        for (float z = farStartZ; z <= planeZ + farRadius; z += farStep) {
            if (std::fabs(x - planeX) <= nearRadius &&
                std::fabs(z - planeZ) <= nearRadius) {
                continue;
            }

            drawVoxelColumn(x, z, farStep);
        }
    }
}
