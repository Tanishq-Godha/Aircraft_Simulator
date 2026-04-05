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
#include "menu.h"
#include "sky.h"
#include "atmosphere.h"
#include "math_utils.h"

void display() {
    if (gameState == 0) {
        drawMenu();
    } else if (gameState == 1) {
        WeatherProfile weather = getWeatherProfile();

        setupSkyClearColor(weather);
        setupAtmosphericFog(weather);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glLoadIdentity();

        // Apply camera transform
        setupCamera();

        // Setup dynamic lighting based on weather and time
        setupAtmosphericLighting(weather);

        // Draw scene
        drawVoxelTerrain();
        drawDetailedJet();
        drawHUD();

        // Draw sky objects
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