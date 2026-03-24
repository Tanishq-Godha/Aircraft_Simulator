#include "camera.h"
#include "globals.h"
#include "math_utils.h"
#include <GL/glut.h>
#include <cmath>

namespace {

float clampf(float value, float minValue, float maxValue) {
    if (value < minValue) return minValue;
    if (value > maxValue) return maxValue;
    return value;
}

float approach(float current, float target, float rate, float dt) {
    float alpha = clampf(rate * dt, 0.0f, 1.0f);
    return current + (target - current) * alpha;
}

} // namespace

void setupCamera() {
    static bool initialized = false;
    static int lastMode = -1;
    static int lastTickMs = 0;
    static float smoothCamX = 0.0f, smoothCamY = 0.0f, smoothCamZ = 0.0f;
    static float smoothLookX = 0.0f, smoothLookY = 0.0f, smoothLookZ = 0.0f;
    static float smoothFov = 80.0f;

    int nowMs = glutGet(GLUT_ELAPSED_TIME);
    float dt = 0.016f;
    if (lastTickMs != 0) {
        dt = (nowMs - lastTickMs) * 0.001f;
        dt = clampf(dt, 0.001f, 0.05f);
    }
    lastTickMs = nowMs;

    float speedRatio = clampf(currentSpeed / 920.0f, 0.0f, 1.15f);

    float pRad = degToRad(pitch);
    float yRad = degToRad(yaw * 0.3f); // Reduce camera yaw sensitivity for more stable view

    float fX = std::sin(yRad) * std::cos(pRad);
    float fY = std::sin(pRad);
    float fZ = -std::cos(yRad) * std::cos(pRad);

    float targetCamX, targetCamY, targetCamZ;
    float targetLookX, targetLookY, targetLookZ;
    float targetFov;

    if (cameraMode == 0) {
        // Chase mode: camera follows plane from behind relative to plane center origin
        // Use plane position as focal point and maintain a fixed trailing offset.
        float camDist = 30.0f + speedRatio * 20.0f;   // behind the plane
        float camHeight = 8.0f + speedRatio * 2.0f;   // elevated above the plane
        float lookAhead = 70.0f + speedRatio * 150.0f;
        float lookDown = 5.0f;

        // camera is placed behind the plane in world space, plane is treated as origin reference for this mode
        targetCamX = planeX - fX * camDist;
        targetCamY = planeY + camHeight;
        targetCamZ = planeZ - fZ * camDist;

        // look point is ahead of plane along the motion vector, for trajectory following
        targetLookX = planeX + fX * lookAhead;
        targetLookY = planeY + fY * lookAhead - lookDown;
        targetLookZ = planeZ + fZ * lookAhead;

        targetFov = 72.0f + speedRatio * 20.0f;

    } else if (cameraMode == 1) {
        targetCamX = planeX - fX * 500.0f;
        targetCamY = planeY + 200.0f;
        targetCamZ = planeZ - fZ * 500.0f;

        targetLookX = planeX + fX * 180.0f;
        targetLookY = planeY + fY * 180.0f;
        targetLookZ = planeZ + fZ * 180.0f;
        targetFov = 86.0f + speedRatio * 8.0f;

    } else if (cameraMode == 2) {
        targetCamX = planeX + std::cos(yRad) * 25.0f;
        targetCamY = planeY;
        targetCamZ = planeZ + std::sin(yRad) * 25.0f;

        targetLookX = planeX;
        targetLookY = planeY;
        targetLookZ = planeZ;
        targetFov = 72.0f + speedRatio * 18.0f;

    } else {
        targetCamX = planeX + std::sin(yRad) * 25.0f;
        targetCamY = planeY;
        targetCamZ = planeZ - std::cos(yRad) * 25.0f;

        targetLookX = planeX;
        targetLookY = planeY;
        targetLookZ = planeZ;
        targetFov = 78.0f + speedRatio * 14.0f;
    }

    if (!initialized || lastMode != cameraMode) {
        smoothCamX = targetCamX;
        smoothCamY = targetCamY;
        smoothCamZ = targetCamZ;
        smoothLookX = targetLookX;
        smoothLookY = targetLookY;
        smoothLookZ = targetLookZ;
        smoothFov = targetFov;

        initialized = true;
        lastMode = cameraMode;
    } else {
        float camRate = (cameraMode == 0) ? 10.0f : 6.5f;
        float lookRate = (cameraMode == 0) ? 10.0f : 7.0f;
        float fovRate = (cameraMode == 0) ? 7.0f : 5.0f;

        smoothCamX = approach(smoothCamX, targetCamX, camRate, dt);
        smoothCamY = approach(smoothCamY, targetCamY, camRate, dt);
        smoothCamZ = approach(smoothCamZ, targetCamZ, camRate, dt);
        smoothLookX = approach(smoothLookX, targetLookX, lookRate, dt);
        smoothLookY = approach(smoothLookY, targetLookY, lookRate, dt);
        smoothLookZ = approach(smoothLookZ, targetLookZ, lookRate, dt);
        smoothFov = approach(smoothFov, targetFov, fovRate, dt);
    }

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(smoothFov,
                   float(screenW) / float(screenH),
                   1.0f,
                   30000.0f);

    glMatrixMode(GL_MODELVIEW);
    gluLookAt(smoothCamX, smoothCamY, smoothCamZ,
              smoothLookX, smoothLookY, smoothLookZ,
              0.0f, 1.0f, 0.0f);
}
