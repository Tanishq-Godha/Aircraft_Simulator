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

#include "shadow_system.h"

void display() {
    if (gameState == 0) {
        drawMenu();
    } else if (gameState == 1) {
        WeatherProfile weather = getWeatherProfile();
        float lx, ly, lz;
        bool isSun;
        getActiveLightDirection(lx, ly, lz, isSun);

        // 1. SHADOW PASS (Render depth from light POV)
        gShadows.setupLightSpace(lx, ly, lz, planeX, planeY, planeZ);
        gShadows.bindShadowPass();
        
        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadIdentity();
        float orthoSize = 600.0f; 
        glOrtho(-orthoSize, orthoSize, -orthoSize, orthoSize, 100.0f, 15000.0f);
        
        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadIdentity();
        float dist = 8000.0f;
        gluLookAt(planeX + lx * dist, planeY + ly * dist, planeZ + lz * dist,
                  planeX, planeY, planeZ, 0, 1, 0);

        // Draw objects that cast shadows
        drawVoxelTerrain();
        drawDetailedJet();

        glPopMatrix();
        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
        glMatrixMode(GL_MODELVIEW);
        
        // Restore default framebuffer/shader state before clearing the screen
        gShadows.unbind();

        // 2. MAIN PASS (Render shaded scene)
        glViewport(0, 0, screenW, screenH); 

        setupSkyClearColor(weather);
        setupAtmosphericFog(weather);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glLoadIdentity();

        setupCamera();
        float camMatrix[16];
        glGetFloatv(GL_MODELVIEW_MATRIX, camMatrix);
        
        setupAtmosphericLighting(weather);

        // Draw with shaders
        gShadows.bindMainPass(lx, ly, lz, camMatrix);
        drawVoxelTerrain();
        drawDetailedJet();
        
        gShadows.unbind(); 

        // HUD and Sky are non-shaded
        drawHUD();
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