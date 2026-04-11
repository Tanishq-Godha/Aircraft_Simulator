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
    sx = cosf(angle);
    sz = 0.15f;

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
void drawSun(float sunX, float sunY, float sunZ,
             float elevation, const WeatherProfile& weather)
{
    if (sunY <= -0.05f) return;

    float DIST = 9000.0f;

    // ✅ camera-relative (IMPORTANT)
    float cx = planeX + sunX * DIST;
    float cy = planeY + sunY * DIST;
    float cz = planeZ + sunZ * DIST;

    float size = 220.0f; // bigger sun

    glPushAttrib(GL_ENABLE_BIT);
    glDisable(GL_LIGHTING);
    glDisable(GL_FOG);
    glDisable(GL_DEPTH_TEST);

    float warm = 1.0f - std::min(1.0f, fabsf(sunY));
    float r = 1.0f;
    float g = 0.9f - warm * 0.4f;
    float b = 0.7f - warm * 0.5f;

    glColor3f(r, g, b);

    glPushMatrix();
    glTranslatef(cx, cy, cz);
    glutSolidSphere(size, 40, 40);
    glPopMatrix();

    glEnable(GL_DEPTH_TEST);
    glPopAttrib();
}

// ================= LIGHTING =================
void setupAtmosphericLighting(const WeatherProfile& weather)
{
    float sx, sy, sz;
    getSunDirection(sx, sy, sz);

    float intensity = std::max(0.2f, sy);

    GLfloat pos[] = {-sx, -sy, -sz, 0.0f};

    GLfloat diffuse[] = {
        intensity,
        intensity * 0.95f,
        intensity * 0.85f,
        1.0f
    };

    GLfloat ambient[] = {
        0.2f + intensity * 0.3f,
        0.2f + intensity * 0.3f,
        0.25f + intensity * 0.3f,
        1.0f
    };

    glLightfv(GL_LIGHT0, GL_POSITION, pos);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
    glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);

    glEnable(GL_LIGHT0);
    glEnable(GL_LIGHTING);
}