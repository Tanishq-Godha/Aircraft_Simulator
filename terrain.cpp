#include "terrain.h"
#include "globals.h"
#include "math_utils.h"
#include <GL/glut.h>
#include <cmath>

#include "terrain.h"
#include "globals.h"
#include "math_utils.h"
#include <GL/glut.h>
#include <cmath>

float getVoxelHeight(float x, float z);

namespace {

void drawTexturedCube(float x, float y, float z, float width, float height, float depth, float r, float g, float b) {
    glPushMatrix();
    glTranslatef(x, y, z);
    glScalef(width, height, depth);
    
    float hs = 0.5f; // half-size for unit cube
    
    float variation = 0.2f;
    float darkR = r * (1.0f - variation);
    float darkG = g * (1.0f - variation);
    float darkB = b * (1.0f - variation);
    
    float lightR = std::min(1.0f, r * (1.0f + variation));
    float lightG = std::min(1.0f, g * (1.0f + variation));
    float lightB = std::min(1.0f, b * (1.0f + variation));
    
    glBegin(GL_QUADS);
    
    // Top face (lighter with subtle pattern)
    glColor3f(lightR * 0.9f, lightG * 0.9f, lightB * 0.9f); glVertex3f(-hs, hs, -hs);
    glColor3f(lightR * 1.1f, lightG * 1.1f, lightB * 1.1f); glVertex3f(hs, hs, -hs);
    glColor3f(lightR * 1.1f, lightG * 1.1f, lightB * 1.1f); glVertex3f(hs, hs, hs);
    glColor3f(lightR * 0.9f, lightG * 0.9f, lightB * 0.9f); glVertex3f(-hs, hs, hs);
    
    // Bottom face (darker)
    glColor3f(darkR, darkG, darkB); glVertex3f(-hs, -hs, -hs);
    glColor3f(darkR, darkG, darkB); glVertex3f(-hs, -hs, hs);
    glColor3f(darkR, darkG, darkB); glVertex3f(hs, -hs, hs);
    glColor3f(darkR, darkG, darkB); glVertex3f(hs, -hs, -hs);
    
    // Front face (base color with vertical stripes)
    float stripe1 = std::fabs(std::sin(z * 0.02f)) * 0.3f + 0.7f;
    float stripe2 = std::fabs(std::sin((z + depth) * 0.02f)) * 0.3f + 0.7f;
    glColor3f(r * stripe1, g * stripe1, b * stripe1); glVertex3f(-hs, -hs, hs);
    glColor3f(r * stripe2, g * stripe2, b * stripe2); glVertex3f(hs, -hs, hs);
    glColor3f(r * stripe2, g * stripe2, b * stripe2); glVertex3f(hs, hs, hs);
    glColor3f(r * stripe1, g * stripe1, b * stripe1); glVertex3f(-hs, hs, hs);
    
    // Back face (base color)
    glColor3f(r, g, b); glVertex3f(-hs, hs, -hs);
    glColor3f(r, g, b); glVertex3f(hs, hs, -hs);
    glColor3f(r, g, b); glVertex3f(hs, -hs, -hs);
    glColor3f(r, g, b); glVertex3f(-hs, -hs, -hs);
    
    // Left face (shadowed)
    glColor3f(darkR * 0.8f, darkG * 0.8f, darkB * 0.8f); glVertex3f(-hs, -hs, -hs);
    glColor3f(darkR * 0.8f, darkG * 0.8f, darkB * 0.8f); glVertex3f(-hs, -hs, hs);
    glColor3f(darkR * 0.8f, darkG * 0.8f, darkB * 0.8f); glVertex3f(-hs, hs, hs);
    glColor3f(darkR * 0.8f, darkG * 0.8f, darkB * 0.8f); glVertex3f(-hs, hs, -hs);
    
    // Right face (highlighted)
    glColor3f(lightR * 0.9f, lightG * 0.9f, lightB * 0.9f); glVertex3f(hs, -hs, -hs);
    glColor3f(lightR * 0.9f, lightG * 0.9f, lightB * 0.9f); glVertex3f(hs, hs, -hs);
    glColor3f(lightR * 0.9f, lightG * 0.9f, lightB * 0.9f); glVertex3f(hs, hs, hs);
    glColor3f(lightR * 0.9f, lightG * 0.9f, lightB * 0.9f); glVertex3f(hs, -hs, hs);
    
    glEnd();
    
    glPopMatrix();
}

void drawVoxelColumn(float x, float z, float cellSize) {
    float h = getVoxelHeight(x + cellSize * 0.5f, z + cellSize * 0.5f);
    
    float r, g, b;
    if (h <= BLOCK_SIZE * 2) {
        r = 0.1f; g = 0.35f; b = 0.85f;
    } else if (h <= BLOCK_SIZE * 3) {
        r = 0.85f; g = 0.85f; b = 0.6f;
    } else if (h <= BLOCK_SIZE * 7) {
        r = 0.3f; g = 0.7f; b = 0.25f;
    } else if (h <= BLOCK_SIZE * 11) {
        r = 0.55f; g = 0.55f; b = 0.55f;
    } else {
        r = 0.95f; g = 0.95f; b = 0.95f;
    }
    
    drawTexturedCube(x + cellSize * 0.5f, h * 0.5f, z + cellSize * 0.5f, cellSize, h, cellSize, r, g, b);
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

    if (selectedMap == 1) { // City map - flatter terrain
        freq = 0.0001f;
        amp = 800;
        pers = 0.3f;
    }

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

    // Draw buildings for city map
    if (selectedMap == 1) {
        float buildingStep = BLOCK_SIZE * 4.0f;
        float buildingStartX = floor((planeX - nearRadius) / buildingStep) * buildingStep;
        float buildingStartZ = floor((planeZ - nearRadius) / buildingStep) * buildingStep;

        for (float x = buildingStartX; x <= planeX + nearRadius; x += buildingStep) {
            for (float z = buildingStartZ; z <= planeZ + nearRadius; z += buildingStep) {
                if (std::fabs(x - planeX) > nearRadius * 0.8f || std::fabs(z - planeZ) > nearRadius * 0.8f) continue;
                
                float groundHeight = getVoxelHeight(x, z);
                float buildingHeight = 500.0f + hash(x * 0.001f, z * 0.001f) * 1000.0f;
                
                // Draw building as a tall cube
                drawTexturedCube(x, groundHeight + buildingHeight * 0.5f, z, 
                               BLOCK_SIZE * 2.0f, buildingHeight, BLOCK_SIZE * 2.0f, 
                               0.7f, 0.7f, 0.8f); // Gray buildings
            }
        }
    }
}
