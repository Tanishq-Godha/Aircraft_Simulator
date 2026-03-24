#include <GL/glut.h>
#include <cstring>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#include "globals.h"
#include "terrain.h"
#include "jet.h"
#include "hud.h"
#include "physics.h"
#include "camera.h"
#include "input.h"
#include "init.h"

namespace {

float clampf(float value, float minValue, float maxValue) {
    if (value < minValue) return minValue;
    if (value > maxValue) return maxValue;
    return value;
}

float mixf(float a, float b, float t) {
    return a + (b - a) * t;
}

float hash01(float x, float y, float seed) {
    float h = std::sin(x * 127.1f + y * 311.7f + seed * 74.7f) * 43758.5453123f;
    return h - std::floor(h);
}

int floorToInt(float value) {
    return static_cast<int>(std::floor(value));
}

struct WeatherProfile {
    float cloudiness;
    float haze;
    float storm;
    float sunBoost;
    float starVisibility;
    float fogStart;
    float fogEnd;
    float skyMute;
};

WeatherProfile getWeatherProfile() {
    WeatherProfile weather;

    float primaryCycle = 0.5f + 0.5f * std::sin(lightTimer * 0.045f);
    float secondaryCycle = 0.5f + 0.5f * std::sin(lightTimer * 0.019f + 1.7f);

    weather.cloudiness = clampf(0.35f + primaryCycle * 0.38f + secondaryCycle * 0.24f, 0.0f, 1.0f);
    weather.haze = clampf(0.18f + weather.cloudiness * 0.62f, 0.0f, 1.0f);
    weather.storm = clampf((weather.cloudiness - 0.58f) / 0.42f, 0.0f, 1.0f);
    weather.sunBoost = 1.15f - weather.storm * 0.20f;
    weather.starVisibility = 1.05f - weather.cloudiness * 0.55f;
    weather.fogStart = mixf(6500.0f, 2200.0f, weather.haze);
    weather.fogEnd = mixf(14500.0f, 7600.0f, weather.haze);
    weather.skyMute = 0.12f + weather.storm * 0.38f;

    return weather;
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

void drawChunkedCloudLayer(const WeatherProfile& weather,
                           float chunkSize,
                           int chunkRadius,
                           float windX,
                           float windZ,
                           float minHeight,
                           float heightRange,
                           float baseRadius,
                           float radiusJitter,
                           float baseShade,
                           float stormShade,
                           float alpha,
                           int minClusters,
                           int extraClusters,
                           int minPuffs,
                           int extraPuffs,
                           float seedOffset) {
    float driftX = lightTimer * windX;
    float driftZ = lightTimer * windZ;
    float fieldPlaneX = planeX - driftX;
    float fieldPlaneZ = planeZ - driftZ;

    int centerChunkX = floorToInt(fieldPlaneX / chunkSize);
    int centerChunkZ = floorToInt(fieldPlaneZ / chunkSize);
    float shade = mixf(baseShade, stormShade, weather.storm);

    for (int chunkX = centerChunkX - chunkRadius; chunkX <= centerChunkX + chunkRadius; ++chunkX) {
        for (int chunkZ = centerChunkZ - chunkRadius; chunkZ <= centerChunkZ + chunkRadius; ++chunkZ) {
            float chunkSeedX = static_cast<float>(chunkX);
            float chunkSeedZ = static_cast<float>(chunkZ);
            int clusterCount = minClusters + static_cast<int>(weather.cloudiness * extraClusters);
            clusterCount += static_cast<int>(hash01(chunkSeedX, chunkSeedZ, seedOffset) * 2.0f);

            for (int cluster = 0; cluster < clusterCount; ++cluster) {
                float clusterSeed = seedOffset + static_cast<float>(cluster) * 9.13f;
                float localX = hash01(chunkSeedX, chunkSeedZ, clusterSeed + 1.0f) * chunkSize;
                float localZ = hash01(chunkSeedX, chunkSeedZ, clusterSeed + 2.0f) * chunkSize;
                float baseX = chunkX * chunkSize + localX + driftX;
                float baseY = minHeight + hash01(chunkSeedX, chunkSeedZ, clusterSeed + 3.0f) * heightRange;
                float baseZ = chunkZ * chunkSize + localZ + driftZ;
                int clusterSize = minPuffs + static_cast<int>(hash01(chunkSeedX, chunkSeedZ, clusterSeed + 4.0f) * extraPuffs);

                drawCloudCluster(baseX,
                                 baseY,
                                 baseZ,
                                 clusterSize,
                                 baseRadius,
                                 radiusJitter,
                                 shade,
                                 alpha,
                                 clusterSeed + chunkSeedX * 3.7f + chunkSeedZ * 5.1f);
            }
        }
    }
}

} // namespace

void reshape(int w, int h) {
    screenW = w;
    screenH = h;
    glViewport(0,0,w,h);
}

void drawText(float x, float y, const char* text, void* font = GLUT_BITMAP_HELVETICA_18) {
    glRasterPos2f(x, y);
    for (const char* c = text; *c; c++) {
        glutBitmapCharacter(font, *c);
    }
}

void drawStrokeText(float x, float y, const char* text, float scale = 0.2f, bool bold = false) {
    glPushMatrix();
    glTranslatef(x, y, 0);
    glScalef(scale, scale, 1.0f);
    if (bold) {
        glLineWidth(2.0f);
        for (const char* c = text; *c; c++) {
            glutStrokeCharacter(GLUT_STROKE_ROMAN, *c);
        }
        glLineWidth(1.0f);
    } else {
        for (const char* c = text; *c; c++) {
            glutStrokeCharacter(GLUT_STROKE_ROMAN, *c);
        }
    }
    glPopMatrix();
}

void drawBackgroundGradient() {
    glBegin(GL_QUADS);
    glColor3f(0.10f, 0.14f, 0.28f);
    glVertex2f(0.0f, 0.0f);
    glVertex2f(screenW, 0.0f);
    glColor3f(0.04f, 0.18f, 0.35f);
    glVertex2f(screenW, screenH);
    glVertex2f(0.0f, screenH);
    glEnd();
}

void drawButton(int x1, int y1, int width, int height, const char* label, bool hover = false) {
    float r = hover ? 0.9f : 1.0f;
    float g = hover ? 0.9f : 1.0f;
    float b = hover ? 0.9f : 1.0f;

    // button border/background
    glColor4f(0.85f, 0.95f, 1.0f, 0.15f);
    glBegin(GL_QUADS);
    glVertex2f(x1 - 3, y1 - 3);
    glVertex2f(x1 + width + 3, y1 - 3);
    glVertex2f(x1 + width + 3, y1 + height + 3);
    glVertex2f(x1 - 3, y1 + height + 3);
    glEnd();

    glColor3f(r, g, b);
    glBegin(GL_QUADS);
    glVertex2f(x1, y1);
    glVertex2f(x1 + width, y1);
    glVertex2f(x1 + width, y1 + height);
    glVertex2f(x1, y1 + height);
    glEnd();

    float textWidth = float(std::strlen(label) * 9);
    float textX = x1 + (width - textWidth) * 0.5f;
    float textY = y1 + (height - 18) * 0.5f + 4;

    glColor3f(0.0f, 0.0f, 0.0f);
    drawText(textX, textY, label, GLUT_BITMAP_HELVETICA_18);
}

bool pointInRect(int px, int py, int x1, int y1, int width, int height) {
    return px >= x1 && px <= x1 + width && py >= y1 && py <= y1 + height;
}

void mouseClick(int button, int state, int x, int y) {
    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
        int mouseY = screenH - y; // Convert to ortho coordinates
        
        if (gameState == 0) { // Menu
            int buttonWidth = 250;
            int buttonHeight = 50;
            int startY = screenH / 2 + 20;
            
            // Start Game button
            if (x >= screenW/2 - buttonWidth/2 && x <= screenW/2 + buttonWidth/2 &&
                mouseY >= startY && mouseY <= startY + buttonHeight) {
                gameState = 1;
            }
            // Controls button
            else if (x >= screenW/2 - buttonWidth/2 && x <= screenW/2 + buttonWidth/2 &&
                     mouseY >= startY - 70 && mouseY <= startY - 20) {
                gameState = 2;
            }
            // Select Map button
            else if (x >= screenW/2 - buttonWidth/2 && x <= screenW/2 + buttonWidth/2 &&
                     mouseY >= startY - 140 && mouseY <= startY - 90) {
                gameState = 3;
            }
            // Exit button
            else if (x >= screenW - 130 && x <= screenW - 10 && mouseY >= 10 && mouseY <= 50) {
                exit(0);
            }
        } else if (gameState == 2) { // Controls
            if (x >= 50 && x <= 150 && mouseY >= 50 && mouseY <= 80) {
                gameState = 0; // Back button
            }
        } else if (gameState == 3) { // Map select
            // Default Terrain button
            if (x >= screenW/2 - 125 && x <= screenW/2 + 125 &&
                mouseY >= screenH/2 + 20 && mouseY <= screenH/2 + 70) {
                selectedMap = 0;
                gameState = 0;
            }
            // City Map button
            else if (x >= screenW/2 - 125 && x <= screenW/2 + 125 &&
                     mouseY >= screenH/2 - 40 && mouseY <= screenH/2 + 10) {
                selectedMap = 1;
                gameState = 0;
            }
            // Back button
            else if (x >= 50 && x <= 170 && mouseY >= 50 && mouseY <= 90) {
                gameState = 0;
            }
        }
    }
}

void drawMenu() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();
    
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, screenW, 0, screenH);
    glMatrixMode(GL_MODELVIEW);

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);

    drawBackgroundGradient();

    int buttonWidth = 250;
    int buttonHeight = 50;
    int centerX = screenW / 2;
    int startY = screenH / 2 + 20;

    glColor3f(1.0f, 1.0f, 1.0f);
    drawStrokeText(centerX - 250, startY + 80, "Aircraft Simulator", 0.4f, true);

    drawButton(centerX - buttonWidth / 2, startY, buttonWidth, buttonHeight, "Start Game");
    drawButton(centerX - buttonWidth / 2, startY - 70, buttonWidth, buttonHeight, "Controls");
    drawButton(centerX - buttonWidth / 2, startY - 140, buttonWidth, buttonHeight, "Select Map");

    drawButton(screenW - 130, 10, 120, 40, "Exit");

    glutSwapBuffers();
}

void drawControls() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();
    
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, screenW, 0, screenH);
    glMatrixMode(GL_MODELVIEW);
    
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    
    drawBackgroundGradient();

    glColor3f(1.0f, 1.0f, 1.0f);
    drawStrokeText(60, screenH - 60, "Game Controls", 0.25f, true);

    int rowStart = screenH - 110;
    int step = 30;
    drawText(60, rowStart, "WASD - Turn");
    drawText(60, rowStart - step, "+/- - Throttle");
    drawText(60, rowStart - 2*step, "G - Landing Gear");
    drawText(60, rowStart - 3*step, "[/] - Flaps");
    drawText(60, rowStart - 4*step, "V - Camera Mode");
    drawText(60, rowStart - 5*step, "P - Reset");
    drawText(60, rowStart - 6*step, "M - Return to Menu");
    drawText(60, rowStart - 7*step, "ESC - Exit");

    drawButton(50, 50, 100, 30, "Back");
    
    glutSwapBuffers();
}

void drawMapSelect() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();
    
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, screenW, 0, screenH);
    glMatrixMode(GL_MODELVIEW);
    
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    
    drawBackgroundGradient();

    glColor3f(1.0f, 1.0f, 1.0f);
    drawStrokeText((screenW - 400) / 2, screenH/2 + 100, "Select Map", 0.35f, true);

    drawButton(screenW/2 - 125, screenH/2 + 20, 250, 50, "Default Terrain");
    drawButton(screenW/2 - 125, screenH/2 - 40, 250, 50, "City Map");

    drawButton(50, 50, 120, 40, "Back");
    
    glutSwapBuffers();
}

void drawSky(const WeatherProfile& weather) {
    glDisable(GL_LIGHTING); // Sky objects don't need lighting

    if (gameTime >= 6.0f && gameTime < 19.0f) {
        // Daytime: sun and clouds (13 hours day)
        float t = (gameTime - 6.0f) / 13.0f;
        float sunElev = std::sin(t * M_PI);
        float sunAzim = t * M_PI;
        float sunX = 10000.0f * std::cos(sunElev) * std::sin(sunAzim);
        float sunY = 10000.0f * std::sin(sunElev);
        float sunZ = 10000.0f * std::cos(sunElev) * std::cos(sunAzim);

        // Draw a larger, brighter sun with a soft glow.
        glPushMatrix();
        glTranslatef(sunX, sunY, sunZ);
        glColor4f(1.0f, 0.92f, 0.55f, 0.18f);
        glutSolidSphere(720.0f, 26, 26);
        glColor4f(1.0f, 0.96f, 0.70f, 0.28f);
        glutSolidSphere(500.0f, 24, 24);
        glColor3f(1.0f, 0.98f, 0.85f);
        glutSolidSphere(320.0f + weather.sunBoost * 45.0f, 24, 24);
        glPopMatrix();

        // Clouds now live in world-space chunks, so they stream past the plane.
        drawChunkedCloudLayer(weather,
                              2600.0f,
                              3,
                              16.0f + weather.storm * 10.0f,
                              7.0f + weather.storm * 5.0f,
                              2500.0f + weather.haze * 220.0f,
                              900.0f,
                              280.0f,
                              85.0f,
                              0.98f,
                              0.70f,
                              0.88f,
                              1,
                              2,
                              6,
                              5,
                              11.0f);
    } else {
        // Nighttime: moon and stars (11 hours night)
        float t = (gameTime < 6.0f ? gameTime + 11.0f : gameTime - 19.0f) / 11.0f;
        float moonElev = std::sin(t * M_PI);
        float moonAzim = t * M_PI;
        float moonX = 5000.0f * std::cos(moonElev) * std::sin(moonAzim);
        float moonY = 5000.0f * std::sin(moonElev);
        float moonZ = 5000.0f * std::cos(moonElev) * std::cos(moonAzim);

        // Draw moon
        glPushMatrix();
        glTranslatef(moonX, moonY, moonZ);
        glColor3f(0.9f, 0.9f, 1.0f);
        glutSolidSphere(150.0f, 20, 20);
        glPopMatrix();

        // Draw many more stars, with visibility tied to weather.
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

        drawChunkedCloudLayer(weather,
                              2800.0f,
                              3,
                              10.0f + weather.storm * 8.0f,
                              4.5f + weather.storm * 3.5f,
                              2450.0f,
                              700.0f,
                              220.0f,
                              60.0f,
                              0.76f,
                              0.50f,
                              0.82f,
                              1,
                              1,
                              5,
                              4,
                              29.0f);
    }

    glEnable(GL_LIGHTING);
}

void display() {
    if (gameState == 0) {
        drawMenu();
    } else if (gameState == 1) {
        WeatherProfile weather = getWeatherProfile();

        // Darken sky at night and adjust clear color according to time (13h day / 11h night)
        if (gameTime >= 19.0f || gameTime < 6.0f) {
            float hazeTint = weather.haze * 0.10f;
            glClearColor(0.02f + hazeTint,
                         0.05f + hazeTint,
                         0.12f + hazeTint * 1.4f,
                         1.0f);
        } else {
            float t = (gameTime - 6.0f) / 13.0f;
            float blend = std::sin(t * M_PI);
            float clearR = 0.1f + 0.4f * blend;
            float clearG = 0.18f + 0.45f * blend;
            float clearB = 0.28f + 0.6f * blend;
            float overcastR = 0.42f + blend * 0.08f;
            float overcastG = 0.50f + blend * 0.10f;
            float overcastB = 0.62f + blend * 0.12f;
            glClearColor(mixf(clearR, overcastR, weather.skyMute),
                         mixf(clearG, overcastG, weather.skyMute),
                         mixf(clearB, overcastB, weather.skyMute),
                         1.0f);
        }

        GLfloat fogColor[] = {
            0.20f + weather.haze * 0.28f,
            0.30f + weather.haze * 0.27f,
            0.42f + weather.haze * 0.24f,
            1.0f
        };
        glFogfv(GL_FOG_COLOR, fogColor);
        glFogf(GL_FOG_START, weather.fogStart);
        glFogf(GL_FOG_END, weather.fogEnd);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glLoadIdentity();

        // 1️⃣ Apply camera transform
        setupCamera();

        // 2️⃣ Dynamic lighting based on day/night cycle
        float envFactor;
        bool isDay = gameTime >= 6.0f && gameTime < 19.0f;
        if (isDay) {
            // Daytime: sun directional light (13 hours day)
            float t = (gameTime - 6.0f) / 13.0f; // 0 to 1
            float sunElev = std::sin(t * M_PI);
            float sunAzim = t * M_PI;
            float sx = std::cos(sunElev) * std::sin(sunAzim);
            float sy = std::sin(sunElev);
            float sz = std::cos(sunElev) * std::cos(sunAzim);

            // Sun glow intensity (strong at midday, lower near horizon)
            envFactor = sunElev > 0.2f ? sunElev : 0.2f;
            float sunIntensity = (0.35f + envFactor * 0.95f) * weather.sunBoost;
            float ambientStrength = 0.24f + envFactor * 0.28f - weather.storm * 0.05f;
            GLfloat sunColor[] = {1.0f, 0.98f, 0.92f, 1.0f};
            GLfloat sunDiffuse[] = {sunColor[0] * sunIntensity, sunColor[1] * sunIntensity, sunColor[2] * sunIntensity, 1.0f};
            GLfloat sunSpec[] = {sunColor[0] * 1.15f, sunColor[1] * 1.10f, sunColor[2] * 1.05f, 1.0f};
            GLfloat sunAmbient[] = {ambientStrength, ambientStrength * 1.02f, ambientStrength * 1.05f, 1.0f};
            GLfloat sunPosition[] = {-sx, -sy, -sz, 0.0f};

            glLightfv(GL_LIGHT0, GL_POSITION, sunPosition);
            glLightfv(GL_LIGHT0, GL_DIFFUSE, sunDiffuse);
            glLightfv(GL_LIGHT0, GL_SPECULAR, sunSpec);
            glLightfv(GL_LIGHT0, GL_AMBIENT, sunAmbient);

            // Simulate soft cloud brightness by adjusting ambient scene
            glLightModelfv(GL_LIGHT_MODEL_AMBIENT, sunAmbient);

            // No moon at daytime
            glDisable(GL_LIGHT1);
        } else {
            // Nighttime: moon as a weaker light source, and stars as scattered ambient (11 hours night)
            float t = (gameTime < 6.0f ? gameTime + 11.0f : gameTime - 19.0f) / 11.0f;
            float moonElev = std::sin(t * M_PI);
            float moonAzim = t * M_PI;
            float mx = std::cos(moonElev) * std::sin(moonAzim);
            float my = std::sin(moonElev);
            float mz = std::cos(moonElev) * std::cos(moonAzim);

            envFactor = 0.15f + 0.6f * moonElev; // fade with moon altitude
            envFactor = envFactor > 0.05f ? envFactor : 0.05f;
            float moonStrength = envFactor * (0.75f + weather.starVisibility * 0.25f);
            GLfloat moonColor[] = {0.5f, 0.55f, 0.62f, 1.0f};
            GLfloat moonDiffuse[] = {moonColor[0] * moonStrength, moonColor[1] * moonStrength, moonColor[2] * moonStrength, 1.0f};
            GLfloat moonSpec[] = {moonColor[0] * 0.7f, moonColor[1] * 0.7f, moonColor[2] * 0.7f, 1.0f};
            GLfloat moonAmbient[] = {
                0.035f + 0.035f * moonElev + weather.starVisibility * 0.01f,
                0.04f + 0.04f * moonElev + weather.starVisibility * 0.01f,
                0.06f + 0.04f * moonElev + weather.starVisibility * 0.015f,
                1.0f
            };

            GLfloat moonPosition[] = {-mx, -my, -mz, 0.0f};
            glLightfv(GL_LIGHT1, GL_POSITION, moonPosition);
            glLightfv(GL_LIGHT1, GL_DIFFUSE, moonDiffuse);
            glLightfv(GL_LIGHT1, GL_SPECULAR, moonSpec);
            glLightfv(GL_LIGHT1, GL_AMBIENT, moonAmbient);

            glEnable(GL_LIGHT1);

            // Stars create low ambient base light
            GLfloat starAmbient[] = {
                0.015f + weather.starVisibility * 0.02f,
                0.02f + weather.starVisibility * 0.025f,
                0.04f + weather.starVisibility * 0.03f,
                1.0f
            };
            glLightModelfv(GL_LIGHT_MODEL_AMBIENT, starAmbient);

            // Disable direct sun light
            glDisable(GL_LIGHT0);
        }

        glEnable(GL_LIGHTING);
        if (isDay) glEnable(GL_LIGHT0);
        else glDisable(GL_LIGHT0);
    

        // 3️⃣ Draw scene
        drawVoxelTerrain();
        drawDetailedJet();
        drawHUD();

        // 4️⃣ Draw sky objects
        drawSky(weather);

        glutSwapBuffers();
    } else if (gameState == 2) {
        drawControls();
    } else if (gameState == 3) {
        drawMapSelect();
    }
}

int main(int argc, char** argv) {

    glutInit(&argc, argv);

    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB |
                        GLUT_DEPTH | GLUT_MULTISAMPLE);

    glutInitWindowSize(screenW, screenH);
    glutCreateWindow("Voxel Flight Modular");

    init();

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutIdleFunc(updatePhysics);
    glutKeyboardFunc(keyDown);
    glutKeyboardUpFunc(keyUp);
    glutSpecialFunc(specialKeyDown);
    glutSpecialUpFunc(specialKeyUp);
    glutMouseFunc(mouseClick);

    glutMainLoop();
}
