#include "sky.h"
#include "atmosphere.h"
#include "globals.h"
#include "math_utils.h"
#include <GL/glut.h>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ================= ORIGINAL CLOUD SYSTEM =================
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
        float shade = clampf(baseShade + (hash01(seedBase, i, 5.0f) - 0.5f) * 0.1f, 0.35f, 1.0f);

        glPushMatrix();
        glTranslatef(baseX + offsetX, baseY + offsetY, baseZ + offsetZ);
        float sx, sy, sz;
getSunDirection(sx, sy, sz);

// simple lighting: brighter if facing sun
float light = 0.6f + 0.4f * sy;

glColor4f(shade * light,
          shade * light,
          (shade + 0.05f) * light,
          alpha);
        glutSolidSphere(radius, 14, 14);
        glPopMatrix();
    }
}

struct CloudLayerConfig {
    float chunkSize;
    int chunkRadius;
    float windX;
    float windZ;
    float minHeight;
    float heightRange;
    float baseRadius;
    float radiusJitter;
    float shade;
    float alpha;
};

CloudLayerConfig defaultDayClouds = {
    2600.0f, 3, 16.0f, 7.0f,
    2500.0f, 900.0f,
    280.0f, 85.0f,
    0.98f, 0.88f
};

CloudLayerConfig defaultNightClouds = {
    2800.0f, 3, 10.0f, 4.5f,
    2450.0f, 700.0f,
    220.0f, 60.0f,
    0.76f, 0.82f
};

void drawChunkedCloudLayer(const WeatherProfile& weather, const CloudLayerConfig& config)
{
    float driftX = lightTimer * config.windX;
    float driftZ = lightTimer * config.windZ;

    int cx = floorToInt((planeX - driftX) / config.chunkSize);
    int cz = floorToInt((planeZ - driftZ) / config.chunkSize);

    for (int x = cx - config.chunkRadius; x <= cx + config.chunkRadius; ++x) {
        for (int z = cz - config.chunkRadius; z <= cz + config.chunkRadius; ++z) {

            float baseX = x * config.chunkSize + driftX;
            float baseZ = z * config.chunkSize + driftZ;
            float baseY = config.minHeight + hash01(x, z, 3.0f) * config.heightRange;

            int clusters = 1 + (int)(weather.cloudiness * 2);

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

} // namespace

// ================= WEATHER =================
WeatherProfile getWeatherProfile()
{
    WeatherProfile w{};
    w.cloudiness = 0.4f;
    w.haze = 0.2f;
    w.storm = 0.0f;
    w.sunBoost = 1.0f;
    w.starVisibility = 1.0f;
    w.fogStart = 5000.0f;
    w.fogEnd = 12000.0f;
    w.skyMute = 0.0f;
    return w;
}

// ================= SKY =================
void drawSky(const WeatherProfile& weather)
{
    glDisable(GL_LIGHTING);

    float sx, sy, sz;
    getSunDirection(sx, sy, sz);

    // DAY
    if (gameTime >= 5.5f && gameTime < 19.0f) {
        drawSun(sx, sy, sz, sy, weather);
        drawChunkedCloudLayer(weather, defaultDayClouds);
    }

    // NIGHT
    if (gameTime >= 18.5f || gameTime < 6.5f) {
        float moonX = planeX - sx * 5000.0f;
        float moonY = planeY - sy * 5000.0f;
        float moonZ = planeZ - sz * 5000.0f;

        glPushMatrix();
        glTranslatef(moonX, moonY, moonZ);
        glColor3f(0.9f, 0.9f, 1.0f);
        glutSolidSphere(150.0f, 20, 20);
        glPopMatrix();

        drawChunkedCloudLayer(weather, defaultNightClouds);
    }

    glEnable(GL_LIGHTING);
}