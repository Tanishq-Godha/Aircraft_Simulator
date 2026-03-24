#include <GL/glut.h>

#include "globals.h"
#include "terrain.h"
#include "jet.h"
#include "hud.h"
#include "physics.h"
#include "camera.h"
#include "input.h"
#include "init.h"

void reshape(int w, int h) {
    screenW = w;
    screenH = h;
    glViewport(0,0,w,h);
}

void display() {

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    // 1️⃣ Apply camera transform
    setupCamera();

    // 2️⃣ 🔥 ADD LIGHTING HERE
    GLfloat light_pos[] = {10000.0f, 15000.0f, 5000.0f, 1.0f};
    GLfloat light_diffuse[] = {1.0f, 0.98f, 0.95f, 1.0f};

    glLightfv(GL_LIGHT0, GL_POSITION, light_pos);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);

    glEnable(GL_LIGHT0);
    glEnable(GL_LIGHTING);

    // 3️⃣ Draw scene
    drawVoxelTerrain();
    drawDetailedJet();
    drawHUD();

    glutSwapBuffers();
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

    glutMainLoop();
}
