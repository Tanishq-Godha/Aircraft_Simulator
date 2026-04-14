#include "init.h"
#include "globals.h"
#include "sky.h"
#include <GL/glut.h>
#include <GL/glu.h>
#include <iostream>

GLUquadric* quadric;

void reshape(int w, int h) {
    screenW = w;
    screenH = h;
    glViewport(0, 0, w, h);
}

#include "shadow_system.h"

void init() {
    std::cout << "Initializing OpenGL..." << std::endl;
    glClearColor(0.4f, 0.7f, 1.0f, 1.0f); 
    glEnable(GL_DEPTH_TEST);              
    glEnable(GL_NORMALIZE); 
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
    
    glShadeModel(GL_FLAT); 
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glEnable(GL_FOG); glFogi(GL_FOG_MODE, GL_LINEAR);
    GLfloat fogColor[4] = {0.4f, 0.7f, 1.0f, 1.0f}; 
    glFogfv(GL_FOG_COLOR, fogColor);
    glFogf(GL_FOG_START, 5000.0f); glFogf(GL_FOG_END, 12000.0f);

    std::cout << "Initializing shadow system..." << std::endl;
    if (!gShadows.init(4096)) {
        std::cout << "FAILED: Shadow system initialization failed!" << std::endl;
        exit(1);
    }
    std::cout << "Shadow system initialized successfully." << std::endl;

    quadric = gluNewQuadric(); gluQuadricNormals(quadric, GLU_SMOOTH); 
    for (int i = 0; i < 256; ++i) {
        keys[i] = false;
        specialKeys[i] = false;
    }
    std::cout << "Initialization complete." << std::endl;

    std::cout << "Initializing Sky and Cloud Textures..." << std::endl;
    initSky();
}
