#include "atmosphere.h"
#include "globals.h"
#include "math_utils.h"
#include <GL/glut.h>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ================= SUN DIRECTION =================
void getSunDirection(float& sx, float& sy, float& sz)
{
    float t = (gameTime - 6.0f) / 12.0f;

    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;

    float angle = t * M_PI;

    sy = sinf(angle);
    sz = -cosf(angle); // Rotate orbit to North-South (Runway axis)
    sx = cosf(angle) * 0.3f;      // Slight offset

    float len = sqrtf(sx*sx + sy*sy + sz*sz);
    sx /= len; sy /= len; sz /= len;
}

// ================= SKY COLOR =================
void setupSkyClearColor(const WeatherProfile& weather)
{
    float sx, sy, sz;
    getSunDirection(sx, sy, sz);

    float day = std::max(0.0f, sy);

    float r = mixf(0.02f, 0.45f, day);
    float g = mixf(0.04f, 0.65f, day);
    float b = mixf(0.10f, 0.95f, day);

    float sunset = 1.0f - std::min(1.0f, fabsf(sy) / 0.2f);
    r = mixf(r, 1.0f, sunset * 0.6f);
    g = mixf(g, 0.4f, sunset * 0.6f);
    b = mixf(b, 0.2f, sunset * 0.6f);

    glClearColor(r, g, b, 1.0f);
}

// ================= FOG =================
void setupAtmosphericFog(const WeatherProfile& weather)
{
    GLfloat fogColor[] = {0.5f, 0.6f, 0.7f, 1.0f};
    glFogfv(GL_FOG_COLOR, fogColor);
    glFogf(GL_FOG_START, weather.fogStart);
    glFogf(GL_FOG_END, weather.fogEnd);
}

// ================= SUN DRAW =================
// ================= SUN DRAW (UPGRADED) =================
// ================= SUN DRAW (BIGGER & ENHANCED SUNSET) =================
void drawSun(float sunX, float sunY, float sunZ,
             float elevation, const WeatherProfile& weather)
{
    // Hide sun if it's well below the horizon
    if (sunY <= -0.05f) return;

    float DIST = 9000.0f;

    // Camera-relative position
    float cx = planeX + sunX * DIST;
    float cy = planeY + sunY * DIST;
    float cz = planeZ + sunZ * DIST;

    // --- SUNSET EFFECT CALCULATIONS ---
    // 'warm' goes from 0.0 (high in sky) to 1.0 (at horizon)
    // Multiplying by 4.0 makes the color shift happen smoothly but lower in the sky
    float warm = 1.0f - std::min(1.0f, fabsf(sunY) * 4.0f); 

    // Illusion: Sun appears larger at the horizon
    float baseSize = 450.0f; // Much bigger base size (was 220)
    float size = baseSize * (1.0f + warm * 0.6f); // Up to 60% bigger at sunset

    // Deepen the colors for sunrise/sunset
    float r = 1.0f;
    float g = 0.95f - warm * 0.65f; // Fades from bright yellow to deep orange
    float b = 0.8f - warm * 0.8f;   // Fades out to leave only red/green channels

    glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_LIGHTING);
    glDisable(GL_FOG);
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE); // Prevent sun from writing to depth buffer

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    // --- 1. BILLBOARD VECTORS ---
    float rightX = sunZ, rightY = 0.0f, rightZ = -sunX; 
    float rLen = sqrtf(rightX*rightX + rightZ*rightZ);
    if (rLen > 0.0001f) {
        rightX /= rLen; rightZ /= rLen;
    } else {
        rightX = 1.0f; rightZ = 0.0f; 
    }

    float tUpX = sunY * rightZ - sunZ * rightY;
    float tUpY = sunZ * rightX - sunX * rightZ;
    float tUpZ = sunX * rightY - sunY * rightX;

    // --- 2. DRAW THE GLOWING CORONA (Halo) ---
    // Corona spreads out more during sunset due to atmospheric haze
    float coronaRadius = size * (2.5f + warm * 1.5f);
    int segments = 32;

    glBegin(GL_TRIANGLE_FAN);
    glColor4f(r, g, b, 0.8f); // Bright center
    glVertex3f(cx, cy, cz);
    
    glColor4f(r, g, b, 0.0f); // Fades to transparent
    for (int i = 0; i <= segments; ++i) {
        float angle = (float)i * (2.0f * M_PI / segments);
        float cosA = cosf(angle);
        float sinA = sinf(angle);
        
        glVertex3f(cx + (rightX * cosA + tUpX * sinA) * coronaRadius,
                   cy + (rightY * cosA + tUpY * sinA) * coronaRadius,
                   cz + (rightZ * cosA + tUpZ * sinA) * coronaRadius);
    }
    glEnd();

    // --- 3. DRAW THE SUN RAYS ---
    // Rays get slightly shorter and softer during sunset
    int numRays = 14;
    float rayLength = size * (4.0f - warm * 1.0f);
    float rayWidth = size * 0.15f;
    float rotationOffset = gameTime * 0.2f; 
    
    float rayAlpha = 0.5f - warm * 0.25f; // Less harsh glare at sunset

    glBegin(GL_TRIANGLES);
    for (int i = 0; i < numRays; ++i) {
        float angle = (float)i * (2.0f * M_PI / numRays) + rotationOffset;

        float cosA = cosf(angle);
        float sinA = sinf(angle);

        float dirX = rightX * cosA + tUpX * sinA;
        float dirY = rightY * cosA + tUpY * sinA;
        float dirZ = rightZ * cosA + tUpZ * sinA;

        float px = -rightX * sinA + tUpX * cosA;
        float py = -rightY * sinA + tUpY * cosA;
        float pz = -rightZ * sinA + tUpZ * cosA;

        glColor4f(r, g, b, rayAlpha);
        glVertex3f(cx, cy, cz);

        glColor4f(r, g, b, 0.0f);
        glVertex3f(cx + dirX * rayLength + px * rayWidth,
                   cy + dirY * rayLength + py * rayWidth,
                   cz + dirZ * rayLength + pz * rayWidth);

        glVertex3f(cx + dirX * rayLength - px * rayWidth,
                   cy + dirY * rayLength - py * rayWidth,
                   cz + dirZ * rayLength - pz * rayWidth);
    }
    glEnd();

    // --- 4. DRAW THE SOLID CORE ---
    glDisable(GL_BLEND);
    // Core turns slightly golden/orange at sunset instead of pure white
    glColor3f(1.0f, 1.0f - warm * 0.2f, 0.9f - warm * 0.5f); 
    glPushMatrix();
    glTranslatef(cx, cy, cz);
    glutSolidSphere(size * 0.5f, 30, 30);
    glPopMatrix();

    glPopAttrib();
}
// ================= LIGHTING =================
void setupAtmosphericLighting(const WeatherProfile& weather)
{
    float sx, sy, sz;
    getSunDirection(sx, sy, sz);

    float intensity = std::max(0.1f, sy);
    if (sy < 0.1f && sy > -0.2f) intensity = 0.1f + (sy + 0.2f) * 0.25f;

    float warm = 1.0f - std::min(1.0f, std::fabs(sy) * 3.0f);
    GLfloat pos[] = {-sx, -sy, -sz, 0.0f};

    GLfloat diffuse[] = {
        std::min(1.0f, intensity * 1.15f + warm * 0.45f),
        std::min(1.0f, intensity * 1.15f + warm * 0.10f),
        std::min(1.0f, intensity * 1.15f),
        1.0f
    };

    float ambLevel = 0.15f + std::max(0.0f, sy) * 0.2f;
    GLfloat ambient[] = {
        ambLevel,
        ambLevel,
        ambLevel,
        1.0f
    };

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glLightfv(GL_LIGHT0, GL_POSITION, pos);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
    glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
}