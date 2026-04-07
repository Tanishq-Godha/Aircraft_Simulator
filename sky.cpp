#include "sky.h"
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

void drawCloudCluster(float baseX,
                      float baseY,
                      float baseZ,
                      int clusterSize,
                      float baseRadius,
                      float radiusJitter,
                      float baseShade,
                      float alpha,
                      float seedBase) {
    for (int puff = 0; puff < clusterSize; ++puff) {
        float puffIndex = static_cast<float>(puff);
        float offsetX = (hash01(seedBase, puffIndex, 1.0f) - 0.5f) * 880.0f;
        float offsetY = (hash01(seedBase, puffIndex, 2.0f) - 0.5f) * 180.0f;
        float offsetZ = (hash01(seedBase, puffIndex, 3.0f) - 0.5f) * 740.0f;
        float radius = baseRadius + hash01(seedBase, puffIndex, 4.0f) * radiusJitter;
        float shade = clampf(baseShade + (hash01(seedBase, puffIndex, 5.0f) - 0.5f) * 0.10f, 0.35f, 1.0f);

        glPushMatrix();
        glTranslatef(baseX + offsetX, baseY + offsetY, baseZ + offsetZ);
        glColor4f(shade, shade, shade + 0.04f, alpha);
        glutSolidSphere(radius, 14, 14);
        glPopMatrix();
    }
}

void drawChunkedCloudLayer(const WeatherProfile& weather, const CloudLayerConfig& config) {
    float driftX = lightTimer * (config.windXBase + weather.storm * config.windXStorm);
    float driftZ = lightTimer * (config.windZBase + weather.storm * config.windZStorm);
    float fieldPlaneX = planeX - driftX;
    float fieldPlaneZ = planeZ - driftZ;

    int centerChunkX = floorToInt(fieldPlaneX / config.chunkSize);
    int centerChunkZ = floorToInt(fieldPlaneZ / config.chunkSize);
    float shade = lerp(config.baseShade, config.stormShade, weather.storm);

    for (int chunkX = centerChunkX - config.chunkRadius; chunkX <= centerChunkX + config.chunkRadius; ++chunkX) {
        for (int chunkZ = centerChunkZ - config.chunkRadius; chunkZ <= centerChunkZ + config.chunkRadius; ++chunkZ) {
            float chunkSeedX = static_cast<float>(chunkX);
            float chunkSeedZ = static_cast<float>(chunkZ);
            int clusterCount = config.minClusters + static_cast<int>(weather.cloudiness * config.extraClusters);
            clusterCount += static_cast<int>(hash01(chunkSeedX, chunkSeedZ, config.seedOffset) * 2.0f);

            for (int cluster = 0; cluster < clusterCount; ++cluster) {
                float clusterSeed = config.seedOffset + static_cast<float>(cluster) * 9.13f;
                float localX = hash01(chunkSeedX, chunkSeedZ, clusterSeed + 1.0f) * config.chunkSize;
                float localZ = hash01(chunkSeedX, chunkSeedZ, clusterSeed + 2.0f) * config.chunkSize;
                float baseX = chunkX * config.chunkSize + localX + driftX;
                
                float actualMinHeight = config.minHeightBase + weather.haze * config.minHeightHaze;
                float baseY = actualMinHeight + hash01(chunkSeedX, chunkSeedZ, clusterSeed + 3.0f) * config.heightRange;
                
                float baseZ = chunkZ * config.chunkSize + localZ + driftZ;
                int clusterSize = config.minPuffs + static_cast<int>(hash01(chunkSeedX, chunkSeedZ, clusterSeed + 4.0f) * config.extraPuffs);

                drawCloudCluster(baseX,
                                 baseY,
                                 baseZ,
                                 clusterSize,
                                 config.baseRadius,
                                 config.radiusJitter,
                                 shade,
                                 config.alpha,
                                 clusterSeed + chunkSeedX * 3.7f + chunkSeedZ * 5.1f);
            }
        }
    }
}

const CloudLayerConfig defaultDayClouds = {
    2600.0f, // chunkSize
    3,       // chunkRadius
    16.0f,   // windXBase
    10.0f,   // windXStorm
    7.0f,    // windZBase
    5.0f,    // windZStorm
    2500.0f, // minHeightBase
    220.0f,  // minHeightHaze
    900.0f,  // heightRange
    280.0f,  // baseRadius
    85.0f,   // radiusJitter
    0.98f,   // baseShade
    0.70f,   // stormShade
    0.88f,   // alpha
    0,       // minClusters
    1,       // extraClusters
    4,       // minPuffs
    3,       // extraPuffs
    11.0f    // seedOffset
};

const CloudLayerConfig defaultNightClouds = {
    2800.0f, // chunkSize
    3,       // chunkRadius
    10.0f,   // windXBase
    8.0f,    // windXStorm
    4.5f,    // windZBase
    3.5f,    // windZStorm
    2450.0f, // minHeightBase
    0.0f,    // minHeightHaze
    700.0f,  // heightRange
    220.0f,  // baseRadius
    60.0f,   // radiusJitter
    0.76f,   // baseShade
    0.50f,   // stormShade
    0.82f,   // alpha
    0,       // minClusters
    1,       // extraClusters
    3,       // minPuffs
    2,       // extraPuffs
    29.0f    // seedOffset
};

} // namespace

WeatherProfile getWeatherProfile() {
    WeatherProfile weather;

    float primaryCycle = 0.5f + 0.5f * std::sin(lightTimer * 0.045f);
    float secondaryCycle = 0.5f + 0.5f * std::sin(lightTimer * 0.019f + 1.7f);

    weather.cloudiness = clampf(0.10f + primaryCycle * 0.20f + secondaryCycle * 0.15f, 0.0f, 1.0f);
    weather.haze = clampf(0.10f + weather.cloudiness * 0.50f, 0.0f, 1.0f);
    weather.storm = clampf((weather.cloudiness - 0.40f) / 0.45f, 0.0f, 1.0f);
    weather.sunBoost = 1.15f - weather.storm * 0.20f;
    weather.starVisibility = 1.05f - weather.cloudiness * 0.55f;
    weather.fogStart = lerp(6500.0f, 2200.0f, weather.haze);
    weather.fogEnd = lerp(14500.0f, 7600.0f, weather.haze);
    weather.skyMute = 0.12f + weather.storm * 0.38f;

    return weather;
}

void drawSky(const WeatherProfile& weather) {
    glDisable(GL_LIGHTING);

    if (gameTime >= 6.0f && gameTime < 19.0f) {
        // Daytime: sun and clouds (13 hours day)
        float t = (gameTime - 6.0f) / 13.0f;
        float sunElev = std::sin(t * M_PI);
        float sunAzim = t * M_PI;
        float sunX = 10000.0f * std::cos(sunElev) * std::sin(sunAzim);
        float sunY = 10000.0f * std::sin(sunElev);
        float sunZ = 10000.0f * std::cos(sunElev) * std::cos(sunAzim);

        glPushMatrix();
        glTranslatef(sunX, sunY, sunZ);
        glColor4f(1.0f, 0.92f, 0.55f, 0.18f);
        glutSolidSphere(720.0f, 26, 26);
        glColor4f(1.0f, 0.96f, 0.70f, 0.28f);
        glutSolidSphere(500.0f, 24, 24);
        glColor3f(1.0f, 0.98f, 0.85f);
        glutSolidSphere(320.0f + weather.sunBoost * 45.0f, 24, 24);
        glPopMatrix();

        drawChunkedCloudLayer(weather, defaultDayClouds);
    } else {
        // Nighttime: moon and stars (11 hours night)
        float t = (gameTime < 6.0f ? gameTime + 11.0f : gameTime - 19.0f) / 11.0f;
        float moonElev = std::sin(t * M_PI);
        float moonAzim = t * M_PI;
        float moonX = 5000.0f * std::cos(moonElev) * std::sin(moonAzim);
        float moonY = 5000.0f * std::sin(moonElev);
        float moonZ = 5000.0f * std::cos(moonElev) * std::cos(moonAzim);

        glPushMatrix();
        glTranslatef(moonX, moonY, moonZ);
        glColor3f(0.9f, 0.9f, 1.0f);
        glutSolidSphere(150.0f, 20, 20);
        glPopMatrix();

        int starCount = 180 + static_cast<int>(weather.starVisibility * 180.0f);
        float starBrightness = clampf(0.65f + weather.starVisibility * 0.35f, 0.55f, 1.0f);
        glPointSize(2.0f + weather.starVisibility * 1.1f);
        glBegin(GL_POINTS);
        for (int i = 0; i < starCount; i++) {
            float twinkle = 0.65f + 0.35f * std::sin(lightTimer * 2.0f + i * 0.37f);
            float brightness = clampf(starBrightness * twinkle, 0.40f, 1.0f);
            glColor3f(brightness, brightness, brightness);
            float x = planeX + std::fmod(i * 137.5f + (i % 11) * 410.0f, 22000.0f) - 11000.0f;
            float y = 7600.0f + std::fmod(i * 73.0f + (i % 7) * 120.0f, 2600.0f);
            float z = planeZ + std::fmod(i * 211.0f + (i % 13) * 260.0f, 22000.0f) - 11000.0f;
            glVertex3f(x, y, z);
        }
        glEnd();

        drawChunkedCloudLayer(weather, defaultNightClouds);
    }

    glEnable(GL_LIGHTING);
}
