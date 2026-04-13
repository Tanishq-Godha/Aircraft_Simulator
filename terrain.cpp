#include "terrain.h"
#include "globals.h"
#include "math_utils.h"
#include "atmosphere.h"
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

bool isInAirportArea(float x, float z) {
    return (std::fabs(x) < 2000.0f && std::fabs(z) < 8000.0f);
}

bool isRunway(float x, float z) {
    return (std::fabs(x) < 350.0f && std::fabs(z) < 6500.0f);
}

bool isTaxiway(float x, float z) {
    if (!isInAirportArea(x, z)) return false;
    // Main taxiway parallel to runway
    if (std::fabs(std::fabs(x) - 600.0f) < 80.0f && std::fabs(z) < 6500.0f) return true;
    // Connecting links
    if (std::fabs(x) < 650.0f && fmod(std::fabs(z), 2000.0f) < 150.0f) return true;
    return false;
}

bool isServiceRoad(float x, float z) {
    if (!isInAirportArea(x, z)) return false;
    // Perimeter roads
    if (std::fabs(std::fabs(x) - 1400.0f) < 40.0f && std::fabs(z) < 7500.0f) return true;
    return false;
}

bool isRoad(float x, float z) {
    if (isTaxiway(x, z) || isServiceRoad(x, z)) return true;
    if (isInAirportArea(x, z)) return false;

    float major = BLOCK_SIZE * 20.0f;
    float minor = BLOCK_SIZE * 8.0f;
    
    float ax = std::fabs(x);
    float az = std::fabs(z);

    float dxMajor = std::fmod(ax, major);
    float dzMajor = std::fmod(az, major);
    float dxMinor = std::fmod(ax, minor);
    float dzMinor = std::fmod(az, minor);

    return (dxMajor < BLOCK_SIZE * 2.0f || dzMajor < BLOCK_SIZE * 2.0f ||
            dxMinor < BLOCK_SIZE || dzMinor < BLOCK_SIZE);
}

// --------------------------------------------------
// BUILDINGS LOGIC (Moved up to resolve declaration order)
// --------------------------------------------------

float getBuildingHeight(float x, float z) {
    float dist = sqrt(x*x + z*z);

    float base = BLOCK_SIZE * (2 + hash(x, z) * 6);
    float centerBoost = exp(-dist * 0.0002f);

    return base + centerBoost * BLOCK_SIZE * 12 * hash(z, x);
}

// Helper to find the building root for a given coordinate
void getPotentialBuildingRoot(float x, float z, float& rx, float& rz) {
    rx = std::floor(x / (BLOCK_SIZE * 3.0f) + 0.5f) * (BLOCK_SIZE * 3.0f);
    rz = std::floor(z / (BLOCK_SIZE * 3.0f) + 0.5f) * (BLOCK_SIZE * 3.0f);
}

// Get dimensions for a building at a specific root
void getBuildingDimensionsAtRoot(float rx, float rz, float& w, float& d, float& h) {
    w = BLOCK_SIZE * (2.0f + hash(rx, rz) * 3.5f);
    d = BLOCK_SIZE * (1.5f + hash(rz, rx) * 3.0f);
    h = getBuildingHeight(rx, rz);
}

// Check if a building ACTUALLY spawns at a given root (rx, rz)
// (Matches logic in hasBuilding)
bool doesBuildingRootSpawn(float rx, float rz);

struct BuildingRect {
    float x1, z1, x2, z2;
};

inline bool intersect(const BuildingRect& a, const BuildingRect& b) {
    return (a.x1 < b.x2 && a.x2 > b.x1 &&
            a.z1 < b.z2 && a.z2 > b.z1);
}

void getBuildingFootprint(float rx, float rz, BuildingRect& foot) {
    float w, d, h;
    getBuildingDimensionsAtRoot(rx, rz, w, d, h);
    foot.x1 = rx - w/2.0f;
    foot.x2 = rx + w/2.0f;
    foot.z1 = rz - d/2.0f;
    foot.z2 = rz + d/2.0f;
}

// Higher-level check that prevents overlapping buildings via priority suppression
bool shouldActualBuildingSpawn(float rx, float rz);

// --------------------------------------------------
// HEIGHT
// --------------------------------------------------

float getVoxelHeight(float x, float z) {
    return 400.0f;
}

// Check if a building should exist at this voxel
// Matches logic in drawVoxelTerrain exactly
bool hasBuilding(float x, float z) {
    if (isInAirportArea(x, z)) {
        // Control tower
        if (std::fabs(x - 900.0f) < 50.0f && std::fabs(z - 450.0f) < 50.0f) return true;
        // Hangar Row
        if (std::fabs(x - 1100.0f) < 150.0f && fmod(std::fabs(z), 1200.0f) < 400.0f) return true;
        return false;
    }
    if (isRoad(x, z)) return false;
    float bHash = hash(floor(x / (BLOCK_SIZE*4)), floor(z / (BLOCK_SIZE*4)));
    if (bHash < 0.8f) return false;

    // Check suppression
    float rx, rz;
    getPotentialBuildingRoot(x, z, rx, rz);
    return shouldActualBuildingSpawn(rx, rz);
}

bool hasTree(float x, float z) {
    if (isRoad(x, z) || isRunway(x, z)) return false;
    if (isInAirportArea(x, z)) {
        // Sparse decorative trees
        return (hash(x, z) > 0.985f);
    }
    float cellHash = hash(x, z);
    if (cellHash < 0.25f) return (hash(x*2, z*2) > 0.6f);
    return false;
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
    glNormal3f(0.0f, 1.0f, 0.0f);
    glVertex3f(-hs, hs, -hs); glVertex3f(hs, hs, -hs);
    glVertex3f(hs, hs, hs);   glVertex3f(-hs, hs, hs);

    // bottom
    glNormal3f(0.0f, -1.0f, 0.0f);
    glVertex3f(-hs, -hs, -hs); glVertex3f(-hs, -hs, hs);
    glVertex3f(hs, -hs, hs);   glVertex3f(hs, -hs, -hs);

    // sides
    glNormal3f(0.0f, 0.0f, 1.0f); // front
    glVertex3f(-hs, -hs, hs); glVertex3f(hs, -hs, hs);
    glVertex3f(hs, hs, hs);   glVertex3f(-hs, hs, hs);

    glNormal3f(0.0f, 0.0f, -1.0f); // back
    glVertex3f(-hs, hs, -hs); glVertex3f(hs, hs, -hs);
    glVertex3f(hs, -hs, -hs); glVertex3f(-hs, -hs, -hs);

    glNormal3f(-1.0f, 0.0f, 0.0f); // left
    glVertex3f(-hs, -hs, -hs); glVertex3f(-hs, -hs, hs);
    glVertex3f(-hs, hs, hs);   glVertex3f(-hs, hs, -hs);

    glNormal3f(1.0f, 0.0f, 0.0f); // right
    glVertex3f(hs, -hs, -hs); glVertex3f(hs, hs, -hs);
    glVertex3f(hs, hs, hs);   glVertex3f(hs, -hs, hs);

    glEnd();
    glPopMatrix();
}

// --------------------------------------------------
// TREES
// --------------------------------------------------

void drawTree(float x, float z, float ground) {
    float hBase = hash(x, z);
    float time = glutGet(GLUT_ELAPSED_TIME) * 0.001f;
    
    // Subtle Wind Sway (stronger at the top)
    float swayX = std::sin(time * 0.8f + x * 0.01f) * 6.0f;
    float swayZ = std::cos(time * 0.7f + z * 0.01f) * 6.0f;
    
    float trunkH = 100.0f + hBase * 80.0f;
    float trunkW = 20.0f + hBase * 10.0f;
    
    // --- Trunk ---
    // Slightly wider base
    drawCube(x, ground + trunkH * 0.1f, z, trunkW * 1.4f, trunkH * 0.2f, trunkW * 1.4f, 0.3f, 0.18f, 0.1f);
    // Main shaft
    drawCube(x, ground + trunkH * 0.5f, z, trunkW, trunkH, trunkW, 0.38f, 0.24f, 0.14f);
    
    // --- Foliage ---
    if (hBase > 0.55f) {
        // TYPE A: CONIFER (Pine-like, Tapered)
        int layers = 6; // Increased from 5 to fill gaps
        float foliageStart = ground + trunkH * 0.45f; // Lowered slightly
        float foliageHeightTotal = trunkH * 1.0f;
        
        for (int i = 0; i < layers; ++i) {
            float t = (float)i / (float)(layers - 1);
            float layerY = foliageStart + foliageHeightTotal * t;
            float layerWidth = (1.4f - t) * (100.0f + hBase * 40.0f);
            
            // Sway increases with height (t)
            float offX = swayX * t;
            float offZ = swayZ * t;
            
            float r = 0.05f + t * 0.1f;
            float g = 0.35f + t * 0.15f;
            float b = 0.1f;
            
            // Increased thickness from 45 to 65 to close gaps
            drawCube(x + offX, layerY, z + offZ, layerWidth, 65.0f, layerWidth, r, g, b);
        }
    } else {
        // TYPE B: BROADLEAF (Oak-like, Clustered)
        float crownBaseY = ground + trunkH - 5.0f; // Lowered to overlap trunk
        float crownSize = 100.0f + hBase * 60.0f;
        
        // Move entire crown with sway
        float cx = x + swayX;
        float cz = z + swayZ;
        
        // 5-cluster bunch
        float rBase = 0.1f + hBase * 0.15f;
        float gBase = 0.45f + hBase * 0.1f;
        float bBase = 0.1f;

        // Center cluster
        drawCube(cx, crownBaseY + 45, cz, crownSize, crownSize * 1.0f, crownSize, rBase, gBase, bBase);
        
        // Orbiting clusters
        float orbit = crownSize * 0.45f;
        float oSize = crownSize * 0.85f; // Slightly larger to close gaps
        drawCube(cx + orbit, crownBaseY, cz, oSize, oSize * 0.7f, oSize, rBase*0.8f, gBase*1.1f, bBase);
        drawCube(cx - orbit, crownBaseY, cz, oSize, oSize * 0.7f, oSize, rBase*1.1f, gBase*0.8f, bBase);
        drawCube(cx, crownBaseY, cz + orbit, oSize, oSize * 0.7f, oSize, rBase*0.9f, gBase*0.9f, bBase);
        drawCube(cx, crownBaseY, cz - orbit, oSize, oSize * 0.7f, oSize, rBase*0.75f, gBase*1.05f, bBase);
        
        // Add "Fruit/Highlights"
        if (hBase > 0.4f) {
             drawCube(cx + orbit, crownBaseY + 20, cz + orbit, 15, 15, 15, 0.8f, 0.2f, 0.1f);
             drawCube(cx - orbit, crownBaseY + 10, cz - orbit, 15, 15, 15, 0.8f, 0.2f, 0.1f);
        }
    }
}

// --------------------------------------------------
// Check if a building ACTUALLY spawns at a given root (rx, rz)
// (Matches logic in hasBuilding)
bool doesBuildingRootSpawn(float rx, float rz) {
    if (isInAirportArea(rx, rz)) {
        return (std::fabs(rx - 900.0f) < 50.0f && std::fabs(rz - 450.0f) < 50.0f);
    }
    if (isRoad(rx, rz)) return false;
    float bHash = hash(std::floor(rx / (BLOCK_SIZE * 4.0f)), std::floor(rz / (BLOCK_SIZE * 4.0f)));
    return (bHash >= 0.8f);
}

// Higher-level check that prevents overlapping buildings via priority suppression
bool shouldActualBuildingSpawn(float rx, float rz) {
    if (!doesBuildingRootSpawn(rx, rz)) return false;
    if (isInAirportArea(rx, rz)) return true; // Hand-placed airport area is exempt from suppression

    BuildingRect myFoot;
    getBuildingFootprint(rx, rz, myFoot);
    float myPriority = hash(rx, rz);

    // Scan neighbors (+/- 1 grid step)
    float step = BLOCK_SIZE * 3.0f;
    for (int offX = -1; offX <= 1; ++offX) {
        for (int offZ = -1; offZ <= 1; ++offZ) {
            if (offX == 0 && offZ == 0) continue;

            float nx = rx + offX * step;
            float nz = rz + offZ * step;

            if (doesBuildingRootSpawn(nx, nz)) {
                BuildingRect neighborFoot;
                getBuildingFootprint(nx, nz, neighborFoot);

                if (intersect(myFoot, neighborFoot)) {
                    float neighborPriority = hash(nx, nz);
                    // Use priority to decide: lower priority is suppressed
                    if (neighborPriority > myPriority) return false;
                    // If priorities are equal (extremely rare), break tie with coordinate
                    if (neighborPriority == myPriority && (nx > rx || (nx == rx && nz > rz))) return false;
                }
            }
        }
    }

    return true;
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


void drawAirportTower(float x, float z, float ground) {
    float h = 650.0f;
    drawCube(x, ground + h*0.4f, z, 100, h*0.8f, 100, 0.7f, 0.7f, 0.75f); // Shaft
    drawCube(x, ground + h*0.85f, z, 160, 100, 160, 0.2f, 0.4f, 0.6f); // Glass
    drawCube(x, ground + h + 20, z, 140, 20, 140, 0.6f, 0.6f, 0.6f); // Roof
    drawCube(x + 30, ground + h + 80, z + 30, 4, 120, 4, 0.1f, 0.1f, 0.1f); // Antenna

    // --- Night Obstruction Light ---
    if (gameTime > 18.2f || gameTime < 5.8f) {
        glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT);
        glDisable(GL_LIGHTING);
        // Bright red glowing glass
        drawCube(x, ground + h*0.85f, z, 162, 102, 162, 0.1f, 0.3f, 0.6f);
        // Red beacon at top of antenna
        glColor3f(1.0f, 0.0f, 0.0f);
        drawCube(x + 30, ground + h + 140, z + 30, 12, 12, 12, 1.0f, 0.0f, 0.0f);
        glPopAttrib();
    }
}

void drawHangar(float x, float z, float ground) {
    float w = 280, d = 350, h = 180;
    // Main shell
    drawCube(x, ground + h/2, z, w, h, d, 0.85f, 0.85f, 0.9f);
    // Vaulted roof
    drawCube(x, ground + h, z, w * 0.9f, 40, d, 0.75f, 0.75f, 0.82f);
    // Support door (darker)
    drawCube(x - w/2 + 5, ground + h/2 - 20, z, 10, h * 0.7f, d * 0.8f, 0.3f, 0.3f, 0.3f);

    // --- Night Obstruction Lights ---
    if (gameTime > 18.2f || gameTime < 5.8f) {
        glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT);
        glDisable(GL_LIGHTING);
        glColor3f(0.8f, 0.0f, 0.0f);
        // Corner lights
        drawCube(x-w/2, ground+h+10, z-d/2, 10, 10, 10, 0.8f,0,0);
        drawCube(x+w/2, ground+h+10, z+d/2, 10, 10, 10, 0.8f,0,0);
        glPopAttrib();
    }
}

void drawBuilding(float x, float z, float ground) {
    if (isInAirportArea(x, z)) {
        if (std::fabs(x - 900.0f) < 50.0f) {
            drawAirportTower(x, z, ground);
        } else {
            drawHangar(x, z, ground);
        }
        return;
    }
    float w, d, h;
    getBuildingDimensionsAtRoot(x, z, w, d, h);

    float r = 0.3f + hash(x, z) * 0.15f;
    float g = 0.32f + hash(z, x) * 0.15f;
    float b = 0.34f + hash(x + z, z) * 0.15f;
    drawCube(x, ground + h/2.0f, z, w, h, d, r, g, b);

    // --- NIGHTTIME WINDOWS ---
    bool isNight = (gameTime > 18.2f || gameTime < 5.8f);
    if (isNight) {
        glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT);
        glDisable(GL_LIGHTING); // Make windows "glow"

        // Draw rows of windows on each face
        float winH = 12.0f;
        float winSpacing = 22.0f;
        
        for (float currY = ground + 30.0f; currY < ground + h - 20.0f; currY += winSpacing) {
            // Only light some windows based on a hash
            if (hash(x, currY) > 0.45f) {
                // Front / Back faces
                float winGlow = 0.7f + hash(currY, z) * 0.3f;
                glColor3f(winGlow, winGlow * 0.9f, 0.2f);
                
                // Front
                drawCube(x, currY, z + d/2.0f + 1.0f, w * 0.7f, winH, 2, winGlow, winGlow*0.9f, 0.2f);
                // Back
                drawCube(x, currY, z - d/2.0f - 1.0f, w * 0.7f, winH, 2, winGlow, winGlow*0.9f, 0.2f);
                // Left
                drawCube(x - w/2.0f - 1.0f, currY, z, 2, winH, d * 0.7f, winGlow, winGlow*0.9f, 0.2f);
                // Right
                drawCube(x + w/2.0f + 1.0f, currY, z, 2, winH, d * 0.7f, winGlow, winGlow*0.9f, 0.2f);
            }
        }
        glPopAttrib();
    }

    float windowH = 25.0f, spacing = 50.0f;
    for (float currH = 50.0f; currH < h - 30.0f; currH += spacing) {
        float winR = 0.1f, winG = 0.2f, winB = 0.3f;
        if (hash(x + currH, z) > 0.75f) { winR = 0.95f; winG = 0.85f; winB = 0.5f; }
        drawCube(x, ground + currH, z + d/2 + 2, w * 0.7f, windowH, 3, winR, winG, winB);
        drawCube(x, ground + currH, z - d/2 - 2, w * 0.7f, windowH, 3, winR, winG, winB);
        drawCube(x + w/2 + 2, ground + currH, z, 3, windowH, d * 0.7f, winR, winG, winB);
        drawCube(x - w/2 - 2, ground + currH, z, 3, windowH, d * 0.7f, winR, winG, winB);
    }
    float roofY = ground + h, rType = hash(x * 1.3f, z * 1.3f);
    if (rType > 0.85f) drawCube(x, roofY + 60, z, 6, 120, 6, 0.2f, 0.2f, 0.2f);
    else if (rType > 0.5f) drawCube(x + w*0.2f, roofY + 10, z + d*0.2f, w*0.3f, 25, d*0.3f, 0.5f, 0.5f, 0.5f);
    if (h > BLOCK_SIZE * 10) {
        float tierH = h * 0.65f, tierW = w * 0.6f, tierD = d * 0.6f;
        drawCube(x, ground + tierH + (h - tierH)/2, z, tierW, h - tierH, tierD, r*1.1f, g*1.1f, b*1.1f);
    }
}
// --------------------------------------------------
// SCENE HEIGHT (FIXES CAMERA)
// --------------------------------------------------

float getSceneHeight(float x, float z) {
    float ground = getVoxelHeight(x, z);
    float maxObjH = 0.0f;

    // --- 1. BUILDINGS (3x3 Neighbors scan) ---
    // Check current cell and all 8 surrounding cells for overlapping footprints
    float step = BLOCK_SIZE * 3.0f;
    float baseRx, baseRz;
    getPotentialBuildingRoot(x, z, baseRx, baseRz);

    for (int offX = -1; offX <= 1; ++offX) {
        for (int offZ = -1; offZ <= 1; ++offZ) {
            float rx = baseRx + offX * step;
            float rz = baseRz + offZ * step;

            if (shouldActualBuildingSpawn(rx, rz)) {
                float w, d, h;
                getBuildingDimensionsAtRoot(rx, rz, w, d, h);
                
                // Check if point (x, z) is inside this building's specific footprint
                if (x >= rx - w/2.0f && x <= rx + w/2.0f &&
                    z >= rz - d/2.0f && z <= rz + d/2.0f) {
                    if (h > maxObjH) maxObjH = h;
                }
            }
        }
    }

    // --- 2. TREES (Single voxel check as they are small) ---
    if (hasTree(x, z)) {
        float trunkH = 80.0f + hash(x, z) * 40.0f;
        float treeH = trunkH + 130.0f;
        if (treeH > maxObjH) maxObjH = treeH;
    }

    return ground + maxObjH;
}

// --------------------------------------------------
// MAIN RENDER
// --------------------------------------------------

void drawVoxelTerrain() {
    float radius = 6000.0f, step = BLOCK_SIZE;
    float startX = floor((planeX - radius) / step) * step;
    float startZ = floor((planeZ - radius) / step) * step;

    for (float x = startX; x <= planeX + radius; x += step) {
        for (float z = startZ; z <= planeZ + radius; z += step) {
            float ground = getVoxelHeight(x, z);

            // 1. RUNWAY
            if (isRunway(x, z)) {
                // Runway Surface (Dark Asphalt)
                drawCube(x, ground/2, z, step, ground, step, 0.12f, 0.12f, 0.12f);
                
                // Professional Markings
                // Side Stripes
                if (std::fabs(x) > 325.0f) {
                    drawCube(x, ground + 5, z, 15.0f, 5, step, 1.0f, 1.0f, 1.0f);
                }
                // Centerline (Dashed)
                else if (std::fabs(x) < 4.0f && fmod(std::fabs(z), 600.0f) < 300.0f) {
                    drawCube(x, ground + 5.1f, z, 3.5f, 4, 150.0f, 1.0f, 1.0f, 1.0f);
                }
                // Threshold "Piano keys"
                if (std::fabs(std::fabs(z) - 6200.0f) < 200.0f) {
                    if (fmod(std::fabs(x), 40.0f) < 20.0f) {
                        drawCube(x, ground + 5.2f, z, 8.0f, 4, 350.0f, 1.0f, 1.0f, 1.0f);
                    }
                }
                continue;
            }

            // 2. TAXIWAYS & ROADS
            if (isTaxiway(x, z)) {
                drawCube(x, ground/2, z, step, ground, step, 0.18f, 0.18f, 0.22f);
                // Edge lines (yellow for taxiways)
                if (fmod(std::fabs(z), 400.0f) < 40.0f) {
                    drawCube(x, ground + 5, z, step, 5, 20, 0.9f, 0.8f, 0.1f);
                }
                continue;
            }
            if (isServiceRoad(x, z)) {
                drawCube(x, ground/2, z, step, ground, step, 0.08f, 0.08f, 0.08f);
                continue;
            }
            if (isRoad(x, z)) {
                // Slightly lighter grey base so shader grain has more contrast
                drawCube(x, ground/2, z, step, ground, step, 0.15f, 0.15f, 0.15f);
                continue; 
            }

            // 3. SOLID GROUND BASE
            drawCube(x, ground/2, z, step, ground, step, 0.2f, 0.5f, 0.2f); 

            // 4. OBJECTS
            if (fmod(std::fabs(x), BLOCK_SIZE * 3) < BLOCK_SIZE &&
                fmod(std::fabs(z), BLOCK_SIZE * 3) < BLOCK_SIZE) {
                if (shouldActualBuildingSpawn(x, z)) {
                    drawBuilding(x, z, ground);
                }
            }
            else if (hasTree(x, z)) {
                drawTree(x, z, ground);
            }
        }
    }
}
