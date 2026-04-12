#include "sky.h"
#include "atmosphere.h"
#include "globals.h"
#include "math_utils.h"
#include <GL/glut.h>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ================= CLOUD SYSTEM =================
namespace {

float hash01(float x, float y, float seed) {
    float h = std::sin(x * 127.1f + y * 311.7f + seed * 74.7f) * 43758.5453123f;
    return h - std::floor(h);
}

int floorToInt(float value) {
    return static_cast<int>(std::floor(value));
}

void drawCloudCluster(float baseX, float baseY, float baseZ,
                      int clusterSize, float baseRadius,
                      float radiusJitter, float baseShade,
                      float alpha, float seedBase)
{
    for (int puff = 0; puff < clusterSize; ++puff) {
        float i = (float)puff;

        float offsetX = (hash01(seedBase, i, 1.0f) - 0.5f) * 880.0f;
        float offsetY = (hash01(seedBase, i, 2.0f) - 0.5f) * 180.0f;
        float offsetZ = (hash01(seedBase, i, 3.0f) - 0.5f) * 740.0f;

        float radius = baseRadius + hash01(seedBase, i, 4.0f) * radiusJitter;
        float shade  = clampf(baseShade + (hash01(seedBase, i, 5.0f) - 0.5f) * 0.1f, 0.35f, 1.0f);

        glPushMatrix();
        glTranslatef(baseX + offsetX, baseY + offsetY, baseZ + offsetZ);

        float sx, sy, sz;
        getSunDirection(sx, sy, sz);
        // clamp: night has sy<0, light doesn't go below 0.25 for moon ambient
        float light = clampf(0.35f + 0.55f * sy, 0.25f, 1.0f);

        // Use standard alpha blend — always restore to this before calling
        glColor4f(shade * light, shade * light, (shade + 0.05f) * light, alpha);
        glutSolidSphere(radius, 12, 12);
        glPopMatrix();
    }
}

struct CloudLayerConfig {
    float chunkSize;
    int   chunkRadius;
    float windX, windZ;
    float minHeight, heightRange;
    float baseRadius, radiusJitter;
    float shade, alpha;
};

// Day clouds
CloudLayerConfig defaultDayClouds = {
    2600.0f, 3, 16.0f, 7.0f,
    2500.0f, 900.0f,
    280.0f, 85.0f,
    0.98f, 0.88f
};

// Night clouds: very sparse so stars show through
CloudLayerConfig defaultNightClouds = {
    4200.0f, 2, 8.0f, 3.5f,
    2450.0f, 700.0f,
    175.0f, 55.0f,
    0.60f, 0.50f
};

void drawChunkedCloudLayer(const WeatherProfile& weather, const CloudLayerConfig& config)
{
    // Ensure clean standard blend before drawing clouds
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);

    float driftX = lightTimer * config.windX;
    float driftZ = lightTimer * config.windZ;

    int cx = floorToInt((planeX - driftX) / config.chunkSize);
    int cz = floorToInt((planeZ - driftZ) / config.chunkSize);

    for (int x = cx - config.chunkRadius; x <= cx + config.chunkRadius; ++x) {
        for (int z = cz - config.chunkRadius; z <= cz + config.chunkRadius; ++z) {

            float baseX = x * config.chunkSize + driftX;
            float baseZ = z * config.chunkSize + driftZ;
            float baseY = config.minHeight + hash01(x, z, 3.0f) * config.heightRange;

            // Limit cluster count so night sky doesn't get too busy
            int maxClusters = (config.shade < 0.8f) ? 1 : 2; // night: max 1
            int clusters     = 1 + (int)(weather.cloudiness * maxClusters);

            for (int c = 0; c < clusters; ++c) {
                drawCloudCluster(baseX, baseY, baseZ,
                                 4, config.baseRadius,
                                 config.radiusJitter,
                                 config.shade,
                                 config.alpha,
                                 (float)c * 10.0f);
            }
        }
    }
}

// ─── Star Field ──────────────────────────────────────────────────────────────
struct Star { float x, y, z; float brightness; };
static Star gStars[1100];
static bool gStarsInited = false;

static float lcg(unsigned int& s) {
    s = s * 1664525u + 1013904223u;
    return (s & 0x7fffffff) / (float)0x7fffffff;
}

void initStars() {
    if (gStarsInited) return;
    unsigned int seed = 0xDEADBEEFu;

    for (int i = 0; i < 1100; ++i) {
        float u = lcg(seed);
        float v = lcg(seed);

        // Uniform sphere distribution (Archimedes)
        float theta = 2.0f * (float)M_PI * u;
        float phi   = std::acos(2.0f * v - 1.0f);

        float sx = std::sin(phi) * std::cos(theta);
        float sy = std::cos(phi);
        float sz = std::sin(phi) * std::sin(theta);

        // Milky Way band: bias stars 600-1100 toward a tilted great circle
        if (i >= 600) {
            float bandAngle  = u * 2.0f * (float)M_PI;
            float bandLat    = std::sin(bandAngle) * 0.40f;
            float spreadV    = (lcg(seed) - 0.5f) * 0.16f;
            phi   = (float)M_PI * 0.5f + bandLat + spreadV;
            theta = bandAngle + lcg(seed) * 0.5f;
            sx = std::sin(phi) * std::cos(theta);
            sy = std::cos(phi);
            sz = std::sin(phi) * std::sin(theta);
        }

        gStars[i].x = sx;
        gStars[i].y = sy;
        gStars[i].z = sz;

        float r = lcg(seed);
        if (r < 0.07f)       gStars[i].brightness = 0.88f + lcg(seed) * 0.12f;
        else if (r < 0.28f)  gStars[i].brightness = 0.50f + lcg(seed) * 0.28f;
        else                  gStars[i].brightness = 0.18f + lcg(seed) * 0.22f;
    }
    gStarsInited = true;
}

void drawStarField(float nightBlend) {
    if (nightBlend < 0.01f) return;
    initStars();

    const float STAR_RADIUS = 18000.0f;

    // Save ALL relevant state explicitly
    GLboolean depthWasEnabled;
    GLboolean lightingWasEnabled;
    GLboolean fogWasEnabled;
    GLboolean blendWasEnabled;
    GLboolean depthMaskWas;
    GLint  blendSrc, blendDst;
    GLfloat pointSizeWas;

    glGetBooleanv(GL_DEPTH_TEST,   &depthWasEnabled);
    glGetBooleanv(GL_LIGHTING,     &lightingWasEnabled);
    glGetBooleanv(GL_FOG,          &fogWasEnabled);
    glGetBooleanv(GL_BLEND,        &blendWasEnabled);
    glGetBooleanv(GL_DEPTH_WRITEMASK, &depthMaskWas);
    glGetIntegerv(GL_BLEND_SRC,    &blendSrc);
    glGetIntegerv(GL_BLEND_DST,    &blendDst);
    glGetFloatv(GL_POINT_SIZE,     &pointSizeWas);

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    glDisable(GL_FOG);
    glDepthMask(GL_FALSE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE); // additive glow
    glEnable(GL_POINT_SMOOTH);
    glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);

    // Pass 1: dim small stars
    glPointSize(1.5f);
    glBegin(GL_POINTS);
    for (int i = 0; i < 1100; ++i) {
        if (gStars[i].brightness > 0.70f) continue; // skip bright for this pass
        if (gStars[i].y < -0.12f) continue;         // below horizon

        float b       = gStars[i].brightness * nightBlend;
        float twinkle = 0.90f + 0.10f * std::sin(lightTimer * 3.2f + (float)i * 0.91f);
        b *= twinkle;

        // Warm-white for dim stars
        glColor4f(b * 0.95f, b * 0.90f, b * 0.80f, b);
        glVertex3f(planeX + gStars[i].x * STAR_RADIUS,
                   planeY + gStars[i].y * STAR_RADIUS,
                   planeZ + gStars[i].z * STAR_RADIUS);
    }
    glEnd();

    // Pass 2: bright larger stars
    glPointSize(2.5f);
    glBegin(GL_POINTS);
    for (int i = 0; i < 1100; ++i) {
        if (gStars[i].brightness <= 0.70f) continue;
        if (gStars[i].y < -0.12f) continue;

        float b       = gStars[i].brightness * nightBlend;
        float twinkle = 0.85f + 0.15f * std::sin(lightTimer * 4.1f + (float)i * 1.13f);
        b *= twinkle;

        // Blue-white for bright/hot stars
        glColor4f(b * 0.88f, b * 0.93f, b * 1.00f, b);
        glVertex3f(planeX + gStars[i].x * STAR_RADIUS,
                   planeY + gStars[i].y * STAR_RADIUS,
                   planeZ + gStars[i].z * STAR_RADIUS);
    }
    glEnd();

    // ── Restore ALL state explicitly (don't rely on glPopAttrib) ─────────────
    glPointSize(pointSizeWas);
    glDepthMask(depthMaskWas);

    // Restore blend function BEFORE evaluating blend enabled state
    glBlendFunc((GLenum)blendSrc, (GLenum)blendDst);

    if (!blendWasEnabled)     glDisable(GL_BLEND);
    if (depthWasEnabled)      glEnable(GL_DEPTH_TEST);
    if (lightingWasEnabled)   glEnable(GL_LIGHTING);
    if (fogWasEnabled)        glEnable(GL_FOG);
    glDisable(GL_POINT_SMOOTH);
}

} // namespace

// ================= WEATHER (gentle time-varying, no storm) =================
WeatherProfile getWeatherProfile()
{
    WeatherProfile w{};

    // Slow gentle cycle — period ~6 in-game hours, range is modest
    float t = std::sin(gameTime * 0.52f) * 0.5f + 0.5f; // 0..1

    if (weatherMode == 1) { // CLEAR
        w.cloudiness     = 0.08f;
        w.haze           = 0.02f;
        w.fogStart       = 8000.0f;
        w.fogEnd         = 25000.0f;
    } else if (weatherMode == 2) { // CLOUDY
        w.cloudiness     = 0.85f;
        w.haze           = 0.15f;
        w.fogStart       = 6000.0f;
        w.fogEnd         = 15000.0f;
    } else if (weatherMode == 3) { // FOGGY
        w.cloudiness     = 0.45f;
        w.haze           = 0.90f;
        w.fogStart       = 100.0f; // Fog starts right at the camera
        w.fogEnd         = 4500.0f;
    } else { // DYNAMIC (0)
        w.cloudiness     = mixf(0.15f, 0.55f, t);
        w.haze           = mixf(0.05f, 0.25f, t);
        w.fogStart       = 5000.0f;
        w.fogEnd         = 12000.0f;
    }

    w.storm          = 0.0f;                      // no storm (keeps sky clean)
    w.sunBoost       = 1.0f;
    w.starVisibility = clampf(1.0f - w.cloudiness * 0.65f, 0.30f, 1.0f);
    w.skyMute        = 0.0f;

    return w;
}


// ================= SKY =================
void drawSky(const WeatherProfile& weather)
{
    glDisable(GL_LIGHTING);

    float sx, sy, sz;
    getSunDirection(sx, sy, sz);

    // ── Night blend: 0=full day, 1=full night ──────────────────────────────
    float nightBlend = 0.0f;
    if (gameTime >= 19.0f)       nightBlend = clampf((gameTime - 19.0f) / 1.0f, 0.0f, 1.0f);
    else if (gameTime < 6.0f)    nightBlend = clampf((6.0f - gameTime)  / 1.0f, 0.0f, 1.0f);

    float starVis = nightBlend * weather.starVisibility;

    // ── DAY: sun + day clouds ───────────────────────────────────────────────
    if (gameTime >= 5.5f && gameTime < 19.0f) {
        drawSun(sx, sy, sz, sy, weather);
        drawChunkedCloudLayer(weather, defaultDayClouds);
    }

    // ── NIGHT: moon + night clouds ──────────────────────────────────────────
    if (gameTime >= 18.5f || gameTime < 6.5f) {
        float lx, ly, lz;
        bool isSun;
        getActiveLightDirection(lx, ly, lz, isSun);

        if (!isSun) {
            float dist = 8500.0f;
            float mx = planeX + lx * dist;
            float my = planeY + ly * dist;
            float mz = planeZ + lz * dist;

            // Moon core — no blend
            glDisable(GL_BLEND);
            glDisable(GL_FOG);
            glDisable(GL_DEPTH_TEST);
            glDepthMask(GL_FALSE);

            glPushMatrix();
            glTranslatef(mx, my, mz);
            glColor3f(0.88f, 0.92f, 1.0f);
            glutSolidSphere(160.0f, 30, 30);
            glPopMatrix();

            // Moon glow — additive, then FULLY restored before clouds
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE);   // additive for glow
            glPushMatrix();
            glTranslatef(mx, my, mz);
            glColor4f(0.4f, 0.5f, 0.9f, 0.25f);
            glutSolidSphere(240.0f, 20, 20);
            glPopMatrix();

            // ── Restore standard blend IMMEDIATELY before anything else ──
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glEnable(GL_BLEND);
            glEnable(GL_DEPTH_TEST);
            glDepthMask(GL_TRUE);
            glEnable(GL_FOG);
        }

        // Night clouds — drawn with standard blend (restored above)
        drawChunkedCloudLayer(weather, defaultNightClouds);
    }

    // ── STARS: drawn last so they appear behind clouds but never bleed ─────
    // Stars use additive blend. Drawing AFTER clouds means star light adds
    // on top of clouds (which looks fine — clouds slightly lit by starlight).
    // More importantly it prevents stars' blend mode from leaking into clouds.
    if (starVis > 0.01f) {
        drawStarField(starVis);
    }

    // Restore lighting for the rest of the frame
    glEnable(GL_LIGHTING);
    // Ensure blend is back to standard after drawStarField
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}