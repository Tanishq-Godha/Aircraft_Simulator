#include "terrain.h"
#include "globals.h"
#include "math_utils.h"
#include <GL/glut.h>
#include <cmath>

// --------------------------------------------------
// NOISE
// --------------------------------------------------

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

// --------------------------------------------------
// CITY STRUCTURE
// --------------------------------------------------

bool isRoad(float x, float z) {
    float major = BLOCK_SIZE * 20.0f;
    float minor = BLOCK_SIZE * 8.0f;

    float dxMajor = fmod(fabs(x), major);
    float dzMajor = fmod(fabs(z), major);

    float dxMinor = fmod(fabs(x + sin(z * 0.001f) * 200), minor);
    float dzMinor = fmod(fabs(z + cos(x * 0.001f) * 200), minor);

    return (dxMajor < BLOCK_SIZE * 2 ||
            dzMajor < BLOCK_SIZE * 2 ||
            dxMinor < BLOCK_SIZE ||
            dzMinor < BLOCK_SIZE);
}

// --------------------------------------------------
// HEIGHT
// --------------------------------------------------

float getVoxelHeight(float x, float z) {
    float h = noise(x * 0.0003f, z * 0.0003f) * BLOCK_SIZE * 3;
    return BLOCK_SIZE + h;
}

// --------------------------------------------------
// DRAWING BASE
// --------------------------------------------------

void drawCube(float x, float y, float z,
              float w, float h, float d,
              float r, float g, float b)
{
    glPushMatrix();
    glTranslatef(x, y, z);
    glScalef(w, h, d);

    float hs = 0.5f;

    glBegin(GL_QUADS);
    glColor3f(r, g, b);

    // top
    glVertex3f(-hs, hs, -hs); glVertex3f(hs, hs, -hs);
    glVertex3f(hs, hs, hs);   glVertex3f(-hs, hs, hs);

    // bottom
    glVertex3f(-hs, -hs, -hs); glVertex3f(-hs, -hs, hs);
    glVertex3f(hs, -hs, hs);   glVertex3f(hs, -hs, -hs);

    // sides
    glVertex3f(-hs, -hs, hs); glVertex3f(hs, -hs, hs);
    glVertex3f(hs, hs, hs);   glVertex3f(-hs, hs, hs);

    glVertex3f(-hs, hs, -hs); glVertex3f(hs, hs, -hs);
    glVertex3f(hs, -hs, -hs); glVertex3f(-hs, -hs, -hs);

    glVertex3f(-hs, -hs, -hs); glVertex3f(-hs, -hs, hs);
    glVertex3f(-hs, hs, hs);   glVertex3f(-hs, hs, -hs);

    glVertex3f(hs, -hs, -hs); glVertex3f(hs, hs, -hs);
    glVertex3f(hs, hs, hs);   glVertex3f(hs, -hs, hs);

    glEnd();
    glPopMatrix();
}

// --------------------------------------------------
// TREES
// --------------------------------------------------

void drawTree(float x, float z, float ground) {

    float trunkH = 80 + hash(x, z) * 40;

    drawCube(x, ground + trunkH/2, z,
             20, trunkH, 20,
             0.4f, 0.25f, 0.1f);

    drawCube(x, ground + trunkH + 40, z,
             80, 60, 80,
             0.1f, 0.6f, 0.1f);

    drawCube(x, ground + trunkH + 80, z,
             60, 50, 60,
             0.1f, 0.7f, 0.1f);

    drawCube(x, ground + trunkH + 110, z,
             40, 40, 40,
             0.2f, 0.8f, 0.2f);
}

// --------------------------------------------------
// HOUSES
// --------------------------------------------------

void drawHouse(float x, float z, float ground) {

    float w = BLOCK_SIZE * (0.6f + hash(x, z) * 0.5f);
    float h = BLOCK_SIZE * (1.5f + hash(z, x) * 2.0f);

    drawCube(x, ground + h/2, z,
             w, h, w,
             0.7f, 0.5f, 0.3f);

    drawCube(x, ground + h + 20, z,
             w * 1.2f, 40, w * 1.2f,
             0.5f, 0.1f, 0.1f);
}

// --------------------------------------------------
// BUILDINGS
// --------------------------------------------------

float getBuildingHeight(float x, float z) {
    float dist = sqrt(x*x + z*z);

    float base = BLOCK_SIZE * (2 + hash(x, z) * 6);
    float centerBoost = exp(-dist * 0.0002f);

    return base + centerBoost * BLOCK_SIZE * 12 * hash(z, x);
}

void drawBuilding(float x, float z, float ground) {

    // LOWER DENSITY (only few cells spawn buildings)
    float bHash = hash(floor(x / (BLOCK_SIZE*4)), floor(z / (BLOCK_SIZE*4)));
    if (bHash < 0.8f) return;   // only ~20% spawn

    float h = getBuildingHeight(x, z);

    // BIGGER + VARIABLE BASE AREA
    float baseScale = 2.0f + hash(x, z) * 3.5f;   // large variation
    float w = BLOCK_SIZE * baseScale;
    float d = BLOCK_SIZE * (1.5f + hash(z, x) * 3.0f);

    // realistic colors
    float r = 0.3f + hash(x, z) * 0.15f;
    float g = 0.32f + hash(z, x) * 0.15f;
    float b = 0.34f + hash(x + z, z) * 0.15f;

    // MAIN BUILDING
    drawCube(x, ground + h/2, z, w, h, d, r, g, b);

    // WINDOWS (scaled with building size)
    float windowH = 25.0f;
    float spacing = 50.0f;

    for (float currH = 50.0f; currH < h - 30.0f; currH += spacing) {

        float winR = 0.1f, winG = 0.2f, winB = 0.3f;
        if (hash(x + currH, z) > 0.75f) {
            winR = 0.95f; winG = 0.85f; winB = 0.5f;
        }

        // +Z
        drawCube(x, ground + currH, z + d/2 + 2, w * 0.7f, windowH, 3, winR, winG, winB);
        // -Z
        drawCube(x, ground + currH, z - d/2 - 2, w * 0.7f, windowH, 3, winR, winG, winB);
        // +X
        drawCube(x + w/2 + 2, ground + currH, z, 3, windowH, d * 0.7f, winR, winG, winB);
        // -X
        drawCube(x - w/2 - 2, ground + currH, z, 3, windowH, d * 0.7f, winR, winG, winB);
    }

    // ROOF DETAILS
    float roofY = ground + h;
    float rType = hash(x * 1.3f, z * 1.3f);

    if (rType > 0.85f) {
        drawCube(x, roofY + 60, z, 6, 120, 6, 0.2f, 0.2f, 0.2f); // antenna
    } 
    else if (rType > 0.5f) {
        drawCube(x + w*0.2f, roofY + 10, z + d*0.2f,
                 w*0.3f, 25, d*0.3f,
                 0.5f, 0.5f, 0.5f); // HVAC
    }

    // TIERED TOP (more realistic skyline)
    if (h > BLOCK_SIZE * 10) {
        float tierH = h * 0.65f;
        float tierW = w * 0.6f;
        float tierD = d * 0.6f;

        drawCube(x,
                 ground + tierH + (h - tierH)/2,
                 z,
                 tierW, h - tierH, tierD,
                 r*1.1f, g*1.1f, b*1.1f);
    }
}
// --------------------------------------------------
// SCENE HEIGHT (FIXES CAMERA)
// --------------------------------------------------

float getSceneHeight(float x, float z) {
    float ground = getVoxelHeight(x, z);

    if (isRoad(x, z)) return ground;

    float h = getBuildingHeight(x, z);
    return ground + h;
}

// --------------------------------------------------
// MAIN RENDER
// --------------------------------------------------

void drawVoxelTerrain() {

    float radius = 6000.0f;
    float step = BLOCK_SIZE;

    float startX = floor((planeX - radius) / step) * step;
    float startZ = floor((planeZ - radius) / step) * step;

    for (float x = startX; x <= planeX + radius; x += step) {
        for (float z = startZ; z <= planeZ + radius; z += step) {

            float ground = getVoxelHeight(x, z);

            // ---------------- ROADS ----------------
            if (isRoad(x, z)) {
                drawCube(x, ground/2, z, step, ground, step,
                         0.1f, 0.1f, 0.1f);

                if (fmod(x, BLOCK_SIZE * 4) < BLOCK_SIZE) {
                    drawCube(x, ground + 5, z,
                             step * 0.1f, 5, step * 0.6f,
                             1.0f, 1.0f, 0.2f);
                }
                continue;
            }

            // ---------------- EMPTY / TREES ----------------
            if (hash(x, z) < 0.25f) {
                drawCube(x, ground/2, z, step, ground, step,
                         0.2f, 0.6f, 0.2f);

                if (hash(x*2, z*2) > 0.6f) {
                    drawTree(x, z, ground);
                }
                continue;
            }

            // ---------------- HOUSES ----------------
            if (hash(x+10, z+10) < 0.4f) {
                // drawHouse(x, z, ground);
                continue;
            }

            // ---------------- BUILDINGS ----------------
            if (fmod(x, BLOCK_SIZE * 3) < BLOCK_SIZE &&
                fmod(z, BLOCK_SIZE * 3) < BLOCK_SIZE)
            {
                drawBuilding(x, z, ground);
            }
        }
    }
}
