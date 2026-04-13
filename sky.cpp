#include "sky.h"
#include "atmosphere.h"
#include "globals.h"
#include "math_utils.h"
#include <GL/glut.h>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace {

float hash01(float x, float y, float seed) {
    float h = std::sin(x * 127.1f + y * 311.7f + seed * 74.7f) * 43758.5453123f;
    return h - std::floor(h);
}

int floorToInt(float value) {
    return static_cast<int>(std::floor(value));
}

// Low-overhead Cellular (Worley) Noise for cauliflower textures
float worley(float x, float y, float seed) {
    float minDist = 1.0f;
    int ix = (int)std::floor(x);
    int iy = (int)std::floor(y);
    for (int j = -1; j <= 1; j++) {
        for (int i = -1; i <= 1; i++) {
            float pX = (float)(ix + i);
            float pY = (float)(iy + j);
            float hX = hash01(pX, pY, seed);
            float hY = hash01(pX, pY, seed + 1.23f);
            float dx = (pX + hX) - x;
            float dy = (pY + hY) - y;
            float d = dx*dx + dy*dy;
            if (d < minDist) minDist = d;
        }
    }
    return std::sqrt(minDist);
}

void drawCloudPuff(float x, float y, float z, float radius, float shade, float alpha, float light) {
    glPushMatrix();
    glTranslatef(x, y, z);

    // Billboarding: Extract the ModelView matrix and zero-out rotation
    float modelView[16];
    glGetFloatv(GL_MODELVIEW_MATRIX, modelView);
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            if (i == j) modelView[i * 4 + j] = 1.0f;
            else modelView[i * 4 + j] = 0.0f;
        }
    }
    glLoadMatrixf(modelView);

    // --- Atmospheric Blending (Fix "black" distant clouds) ---
    float distSq = (x-planeX)*(x-planeX) + (y-planeY)*(y-planeY) + (z-planeZ)*(z-planeZ);
    float d = std::sqrt(distSq);
    
    float skyR, skyG, skyB;
    getSkyColor(skyR, skyG, skyB);
    
    // Blend toward sky color based on distance (start 10k, full sky 22k)
    float blend = clampf((d - 10000.0f) / 12000.0f, 0.0f, 1.0f);
    
    float r = shade * light;
    float g = shade * light;
    float b = shade * light; // Neutral Greyish-White
    
    float finalR = mixf(r, skyR, blend);
    float finalG = mixf(g, skyG, blend);
    float finalB = mixf(b, skyB, blend);

    // --- Final Alpha calculation (Distance-based opacity reduction) ---
    // Aggressively reduce opacity for far away clouds to make them misty
    float opacityScale = clampf(1.1f - d / 25000.0f, 0.0f, 1.0f);
    float finalAlpha = alpha * opacityScale;

    glColor4f(finalR, finalG, finalB, finalAlpha);

    float hs = radius;
    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 0.0f); glVertex2f(-hs, -hs);
    glTexCoord2f(1.0f, 0.0f); glVertex2f( hs, -hs);
    glTexCoord2f(1.0f, 1.0f); glVertex2f( hs,  hs);
    glTexCoord2f(0.0f, 1.0f); glVertex2f(-hs,  hs);
    glEnd();

    glPopMatrix();
}

void drawCloudCluster(float baseX, float baseY, float baseZ,
                      int clusterSize, float baseRadius,
                      float radiusJitter, float baseShade,
                      float alpha, float seedBase,
                      float stretchX, float stretchY, float stretchZ)
{
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, cloudTextureId);
    glDepthMask(GL_FALSE);

    // Disable legacy fog during clouds to prevent "black smudges" mismatch
    glDisable(GL_FOG);

    float sx, sy, sz;
    getSunDirection(sx, sy, sz);
    // STICK TO GREYISH-WHITE: Lock light floor much higher (0.85 instead of 0.45)
    float light = clampf(0.70f + 0.30f * sy, 0.85f, 1.0f);

    // High-density puffs for volumetric look
    int density = clusterSize * 6; // Standard clusterSize 4 becomes 24 puffs
    
    for (int puff = 0; puff < density; ++puff) {
        float i = (float)puff;

        // Gaussian-like distribution for a rounded "puffy" cloud body
        // --- Position generation (FIXED CLEAN VERSION) ---

float phi = hash01(seedBase, i, 1.0f) * 2.0f * (float)M_PI;
float cosTheta = 2.0f * hash01(seedBase, i, 2.0f) - 1.0f;
float sinTheta = std::sqrt(1.0f - cosTheta * cosTheta);

// Better distribution
float r = std::pow(hash01(seedBase, i, 3.0f), 1.5f);

// ✅ Declare ALL offsets FIRST (randomized stretch per cluster)
float offsetX = r * sinTheta * std::cos(phi) * 600.0f * stretchX;
float offsetZ = r * sinTheta * std::sin(phi) * 600.0f * stretchZ;
float offsetY = r * cosTheta * 100.0f * stretchY; // Reduced vertical depth (150 -> 100)

// ✅ THEN modify offsetY for flatter bases
if (offsetY < 0.0f) offsetY *= 0.3f; // More aggressive base flattening

        // --- Realistic Lighting Gradient (Greyish White LOCK) ---
        float heightFactor = (offsetY + 150.0f) / 300.0f; 
        float verticalShade = mixf(0.98f, 1.02f, heightFactor); // Extremely bright range

        // Subtle sun-facing boost
        float sunFactor = 0.92f + 0.08f * sy;
        verticalShade *= sunFactor;
        float puffShade = baseShade * verticalShade;

        float radius = (baseRadius * 0.85f) + hash01(seedBase, i, 4.0f) * radiusJitter;
        // Higher alpha for a more "solid" gaseous look
        float puffAlpha = alpha * 0.65f * (1.1f - r * r * 0.9f);

        drawCloudPuff(baseX + offsetX, baseY + offsetY, baseZ + offsetZ, radius, puffShade, puffAlpha, light);
    }

    glDepthMask(GL_TRUE);
    glDisable(GL_TEXTURE_2D);
}

struct CloudLayerConfig {
    float chunkSize;
    int   chunkRadius;
    float windX, windZ;
    float minHeight, heightRange;
    float baseRadius, radiusJitter;
    float shade, alpha;
};

// Day clouds (Scattered Puffy Cumulus)
CloudLayerConfig defaultDayClouds = {
    10000.0f, 3, 16.0f, 7.0f,   
    3800.0f, 1500.0f,           // Higher and thinner altitude band (from 3800 up)
    650.0f, 300.0f,              // Moderated size (850 -> 650)
    1.0f, 0.95f                  
};

// Night clouds: very sparse so stars show through
CloudLayerConfig defaultNightClouds = {
    8000.0f, 2, 8.0f, 3.5f,
    2600.0f, 1000.0f,
    220.0f, 80.0f,
    0.50f, 0.45f
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
            
            // Randomize height within a much wider range for natural sky depth
            float heightOffset = hash01(x, z, 3.0f) * config.heightRange;
            float baseY = config.minHeight + heightOffset;

            // Randomize clusters per chunk
            int clusters = 1 + (int)(weather.cloudiness * 3.0f);
            clusters += (int)(hash01(x, z, 9.0f) * 3.0f); // Variable density
            for (int c = 0; c < clusters; ++c) {
                float cSeed = (float)c * 17.3f;
                // Randomize cluster size and radius
                float sizeScale = 0.6f + hash01(x, z, cSeed + 1.1f) * 1.2f;
                float clusterRadius = config.baseRadius * sizeScale;
                
                // Elliptical shapes (Stronger width-to-height ratio)
                float shapeType = hash01(x, z, cSeed + 9.9f);
                float sX = 1.0f, sY = 1.0f, sZ = 1.0f;

                if (shapeType < 0.85f) { // Elliptical (Very Wide and Very Flat)
                    sX = 1.7f + hash01(x, z, cSeed + 6.1f) * 1.0f;
                    sZ = 1.7f + hash01(x, z, cSeed + 8.2f) * 1.0f;
                    sY = 0.15f + hash01(x, z, cSeed + 7.3f) * 0.2f; // Much thinner (0.3 -> 0.15)
                } else { // Spherical
                    sX = 1.0f; sY = 1.0f; sZ = 1.0f;
                }

                // Stability: Horizon Fading (Aggressive Anti-popping)
                float distToChunk = std::sqrt((baseX-planeX)*(baseX-planeX) + (baseZ-planeZ)*(baseZ-planeZ));
                float maxRad = config.chunkSize * (float)config.chunkRadius;
                // Fade starts at 50% distance and reaches 0 at maxRad
                float fadeAlpha = config.alpha * clampf((maxRad - distToChunk) / (config.chunkSize * 1.5f), 0.0f, 1.0f);

                // Offset individual clusters within the chunk
                float offX = (hash01(x, z, cSeed + 2.2f) - 0.5f) * config.chunkSize * 0.8f;
                float offZ = (hash01(x, z, cSeed + 3.3f) - 0.5f) * config.chunkSize * 0.8f;
                float offY = (hash01(x, z, cSeed + 4.4f) - 0.5f) * 400.0f; // Varied heights within the cluster (600 -> 400)

                drawCloudCluster(baseX + offX, baseY + offY, baseZ + offZ,
                                 4, clusterRadius,
                                 config.radiusJitter * sizeScale,
                                 config.shade,
                                 fadeAlpha,
                                 hash01(x, z, cSeed + 5.5f),
                                 sX, sY, sZ);
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

void initSky() {
    const int texSize = 256; // Increased resolution
    unsigned char data[texSize * texSize * 4];

    for (int y = 0; y < texSize; y++) {
        for (int x = 0; x < texSize; x++) {
            float u = (x / (float)(texSize-1)) - 0.5f;
            float v = (y / (float)(texSize-1)) - 0.5f;
            float dist = std::sqrt(u * u + v * v) * 2.0f; // 0..1
            
            // --- 1. Base Density ---
            float baseAlpha = 1.0f - std::pow(clampf(dist, 0.0f, 1.0f), 4.5f);
            
            // --- 2. Cellular Cauliflower Texture ---
            float nx = x * 8.0f / (float)texSize;
            float ny = y * 8.0f / (float)texSize;
            
            // Multi-layered Worley noise for clumping
            float c1 = 1.0f - worley(nx, ny, 10.0f);
            float c2 = 1.0f - worley(nx*2.2f, ny*2.2f, 20.0f);
            float w = (c1 * 0.7f + c2 * 0.3f);
            
            // High contrast clumping
            w = std::pow(w, 1.8f) * 1.4f;

            // --- 3. Final Composite ---
            float alpha = baseAlpha * w;
            
            // Sharpen the final silhouette
            alpha = clampf((alpha - 0.15f) * 1.8f, 0.0f, 1.0f);

            int idx = (y * texSize + x) * 4;
            data[idx + 0] = 255;
            data[idx + 1] = 255;
            data[idx + 2] = 255;
            data[idx + 3] = (unsigned char)(alpha * 255);
        }
    }

    glGenTextures(1, &cloudTextureId);
    glBindTexture(GL_TEXTURE_2D, cloudTextureId);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texSize, texSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glBindTexture(GL_TEXTURE_2D, 0);
}

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