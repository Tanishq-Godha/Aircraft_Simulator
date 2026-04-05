#include "atmosphere.h"
#include "globals.h"
#include "math_utils.h"
#include <GL/glut.h>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

void setupSkyClearColor(const WeatherProfile& weather) {
    if (gameTime >= 19.0f || gameTime < 6.0f) {
        // Nighttime clear color
        float hazeTint = weather.haze * 0.10f;
        glClearColor(0.02f + hazeTint,
                     0.05f + hazeTint,
                     0.12f + hazeTint * 1.4f,
                     1.0f);
    } else {
        // Daytime clear color with time-based gradient
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
}

void setupAtmosphericFog(const WeatherProfile& weather) {
    GLfloat fogColor[] = {
        0.20f + weather.haze * 0.28f,
        0.30f + weather.haze * 0.27f,
        0.42f + weather.haze * 0.24f,
        1.0f
    };
    glFogfv(GL_FOG_COLOR, fogColor);
    glFogf(GL_FOG_START, weather.fogStart);
    glFogf(GL_FOG_END, weather.fogEnd);
}

void setupAtmosphericLighting(const WeatherProfile& weather) {
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
        float envFactor = sunElev > 0.2f ? sunElev : 0.2f;
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
        glEnable(GL_LIGHT0);
    } else {
        // Nighttime: moon as a weaker light source, and stars as scattered ambient (11 hours night)
        float t = (gameTime < 6.0f ? gameTime + 11.0f : gameTime - 19.0f) / 11.0f;
        float moonElev = std::sin(t * M_PI);
        float moonAzim = t * M_PI;
        float mx = std::cos(moonElev) * std::sin(moonAzim);
        float my = std::sin(moonElev);
        float mz = std::cos(moonElev) * std::cos(moonAzim);

        float envFactor = 0.15f + 0.6f * moonElev; // fade with moon altitude
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
}
