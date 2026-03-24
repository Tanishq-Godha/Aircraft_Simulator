#include "terrain.h"
#include "globals.h"
#include "math_utils.h"
#include <GL/glut.h>
#include <cmath>

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
    float startX = floor(planeX / BLOCK_SIZE) * BLOCK_SIZE - 6000;
    float startZ = floor(planeZ / BLOCK_SIZE) * BLOCK_SIZE - 6000;

    for (float x = startX; x < startX + 12000; x += BLOCK_SIZE) {
        for (float z = startZ; z < startZ + 12000; z += BLOCK_SIZE) {

            float h = getVoxelHeight(x + BLOCK_SIZE/2, z + BLOCK_SIZE/2);

            if (h <= BLOCK_SIZE * 2) glColor3f(0.1,0.35,0.85);
            else if (h <= BLOCK_SIZE * 3) glColor3f(0.85,0.85,0.6);
            else if (h <= BLOCK_SIZE * 7) glColor3f(0.3,0.7,0.25);
            else if (h <= BLOCK_SIZE * 11) glColor3f(0.55,0.55,0.55);
            else glColor3f(0.95,0.95,0.95);

            glPushMatrix();
            glTranslatef(x + BLOCK_SIZE/2, h/2, z + BLOCK_SIZE/2);
            glScalef(BLOCK_SIZE, h, BLOCK_SIZE);
            glutSolidCube(1);
            glPopMatrix();
        }
    }
}