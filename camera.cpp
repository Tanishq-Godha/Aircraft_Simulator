#include "camera.h"
#include "globals.h"
#include "math_utils.h"
#include <GL/glut.h>
#include <cmath>

void setupCamera() {

    float speedRatio = currentSpeed / 600.0f;

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    float fov = (cameraMode == 1) ? 80.0f : 70.0f + speedRatio * 20.0f;

    gluPerspective(fov,
                   (float)screenW / screenH,
                   1.0,
                   30000.0);

    glMatrixMode(GL_MODELVIEW);

    float camX, camY, camZ;
    float lookX, lookY, lookZ;

    float pRad = degToRad(pitch);
    float yRad = degToRad(yaw);

    float fX = sin(yRad) * cos(pRad);
    float fY = -sin(pRad);
    float fZ = -cos(yRad) * cos(pRad);

    if (cameraMode == 0) {           // CHASE
        float camDist = 20 + speedRatio * 15;

        camX = planeX - sin(yRad) * camDist;
        camZ = planeZ + cos(yRad) * camDist;
        camY = planeY + 8;

        lookX = planeX + fX * 50;
        lookY = planeY + fY * 50;
        lookZ = planeZ + fZ * 50;
    }
    else if (cameraMode == 1) {      // COCKPIT
        camX = planeX - fX * 1.5f;
        camY = planeY + 0.8f;
        camZ = planeZ - fZ * 1.5f;

        lookX = planeX + fX * 100;
        lookY = planeY + fY * 100;
        lookZ = planeZ + fZ * 100;
    }
    else if (cameraMode == 2) {      // CINEMATIC
        camX = planeX + cos(yRad) * 40;
        camY = planeY + 5;
        camZ = planeZ + sin(yRad) * 40;

        lookX = planeX;
        lookY = planeY;
        lookZ = planeZ;
    }
    else {                           // FLY-BY
        camX = planeX + sin(yRad) * 40;
        camY = planeY + 5;
        camZ = planeZ - cos(yRad) * 40;

        lookX = planeX;
        lookY = planeY;
        lookZ = planeZ;
    }

    gluLookAt(camX, camY, camZ,
              lookX, lookY, lookZ,
              0, 1, 0);
}