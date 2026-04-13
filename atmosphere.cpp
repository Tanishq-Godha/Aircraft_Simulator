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
    // 6:30 AM = sunrise, 7:00 PM = sunset (12.5 hours of daylight)
    float t = (gameTime - 6.5f) / 12.5f;

    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;

    float angle = t * M_PI;

    sy = sinf(angle);
    sz = -cosf(angle); // Rotate orbit to North-South (Runway axis)
    sx = cosf(angle) * 0.3f;      // Slight offset

    float len = sqrtf(sx*sx + sy*sy + sz*sz);
    sx /= len; sy /= len; sz /= len;
}

void getActiveLightDirection(float& dx, float& dy, float& dz, bool& isSun)
{
    // 6:30 AM to 7:00 PM = daytime with sun
    if (gameTime >= 6.5f && gameTime <= 19.0f) {
        getSunDirection(dx, dy, dz);
        isSun = true;
    } else {
        // Night: use moon (much dimmer lighting)
        float moonTime = gameTime;
        if (moonTime < 6.5f) moonTime += 24.0f;
        float t = (moonTime - 19.0f) / 11.5f;  // 7 PM to 6:30 AM
        float angle = t * M_PI;
        dy = sinf(angle);
        dz = -cosf(angle);
        dx = cosf(angle) * 0.3f;
        float len = sqrtf(dx*dx + dy*dy + dz*dz);
        dx /= len; dy /= len; dz /= len;
        isSun = false;
    }
}

// ================= SKY COLOR =================
void setupSkyClearColor(const WeatherProfile& weather)
{
    // Compute TRUE sun elevation directly from gameTime.
    // 6:30 AM to 7:00 PM = daytime
    float sunElev;
    if (gameTime >= 6.5f && gameTime <= 19.0f) {
        float t = (gameTime - 6.5f) / 12.5f;
        sunElev = std::sin(t * (float)M_PI);   // 0 at dawn/dusk, 1 at noon
    } else {
        float nt = (gameTime > 19.0f) ? (gameTime - 19.0f)
                                      : (gameTime + 5.0f);  // 5 hours from midnight to 6:30 AM
        sunElev = -std::sin((nt / 11.5f) * (float)M_PI); // negative = below horizon
    }

    float r, g, b;

    if (sunElev <= -0.10f) {
        // ── Full night: deep navy fading to near-black ────────────────────
        float depth = clampf((-sunElev - 0.10f) / 0.40f, 0.0f, 1.0f);
        r = mixf(0.04f, 0.01f, depth);
        g = mixf(0.04f, 0.01f, depth);
        b = mixf(0.13f, 0.03f, depth);
    } else if (sunElev < 0.12f) {
        // ── Twilight band (sunrise / sunset) ─────────────────────────────
        float t = (sunElev + 0.10f) / 0.22f;   // 0 = night edge, 1 = day edge
        r = mixf(0.75f, 0.50f, t);
        g = mixf(0.25f, 0.60f, t);
        b = mixf(0.12f, 0.88f, t);
    } else {
        // ── Daytime: light blue sky ───────────────────────────────────────
        float day = clampf(sunElev, 0.0f, 1.0f);
        r = mixf(0.38f, 0.46f, day);
        g = mixf(0.58f, 0.66f, day);
        b = mixf(0.84f, 0.96f, day);
    }

    glClearColor(r, g, b, 1.0f);
}

// ================= FOG =================
void setupAtmosphericFog(const WeatherProfile& weather)
{
    // Match fog color to sky: dark navy at night, hazy blue by day
    float sunElev;
    if (gameTime >= 6.5f && gameTime <= 19.0f) {
        float t = (gameTime - 6.5f) / 12.5f;
        sunElev = std::sin(t * (float)M_PI);
    } else {
        float nt = (gameTime > 19.0f) ? (gameTime - 19.0f) : (gameTime + 5.0f);
        sunElev = -std::sin((nt / 11.5f) * (float)M_PI);
    }
    float day = clampf(sunElev, 0.0f, 1.0f);

    GLfloat fogColor[] = {
        mixf(0.03f, 0.52f, day),
        mixf(0.03f, 0.60f, day),
        mixf(0.10f, 0.70f, day),
        1.0f
    };
    glFogfv(GL_FOG_COLOR, fogColor);
    glFogf(GL_FOG_START, weather.fogStart);
    glFogf(GL_FOG_END,   weather.fogEnd);
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
    float warm = 1.0f - std::min(1.0f, fabsf(sunY) * 3.5f); 
    float peak = std::max(0.0f, sunY); // 1.0 at noon

    // Illusion: Sun appears larger at the horizon
    float baseSize = 480.0f; 
    float size = baseSize * (1.1f + warm * 0.7f); 

    // Deepen the colors for sunrise/sunset
    // At peak (noon), it's pure white/blinding. At sunset, it's deep orange/red.
    float r = 1.0f;
    float g = mixf(0.98f, 0.45f, warm); 
    float b = mixf(0.85f, 0.15f, warm);   

    glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_LIGHTING);
    glDisable(GL_FOG);
    glEnable(GL_DEPTH_TEST);  // Keep depth testing so buildings block sun
    glDepthMask(GL_FALSE);  // Don't write to depth buffer 

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
    // Corona is MUCH brighter and larger at peak daytime
    float coronaRadius = size * (2.8f + peak * 2.2f + warm * 1.5f);
    float coronaAlpha = 0.6f + peak * 0.3f; // Blinding at noon
    int segments = 40;

    glBegin(GL_TRIANGLE_FAN);
    glColor4f(r, g, b, coronaAlpha); 
    glVertex3f(cx, cy, cz);
    
    glColor4f(r, g, b, 0.0f); 
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
    float dx, dy, dz;
    bool isSun;
    getActiveLightDirection(dx, dy, dz, isSun);

    float intensity = std::max(0.12f, dy);
    if (!isSun) intensity *= 0.25f; // Moon is dimmer

    // Sun is much stronger at peak altitude (noon)
    float peakFactor = isSun ? std::max(0.0f, dy) : 0.0f;
    float warmFactor = isSun ? (1.0f - std::min(1.0f, std::fabs(dy) * 3.5f)) : 0.0f;

    GLfloat pos[] = {-dx, -dy, -dz, 0.0f};

    // Diffuse increases with peak altitude
    float diffR = intensity * (1.0f + peakFactor * 0.3f) + warmFactor * 0.5f;
    float diffG = intensity * (1.0f + peakFactor * 0.3f) + warmFactor * 0.15f;
    float diffB = intensity * (1.0f + peakFactor * 0.3f);

    GLfloat diffuse[] = {
        std::min(1.0f, diffR),
        std::min(1.0f, diffG),
        std::min(1.0f, diffB),
        1.0f
    };

    float ambLevel = (isSun ? 0.14f : 0.06f) + std::max(0.0f, dy) * 0.22f;
    GLfloat ambient[] = { ambLevel, ambLevel, ambLevel + (isSun ? 0 : 0.02f), 1.0f };

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glLightfv(GL_LIGHT0, GL_POSITION, pos);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
    glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
}