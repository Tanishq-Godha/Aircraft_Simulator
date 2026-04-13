#include "init.h"
#include "globals.h"
#include <GL/glut.h>
#include <GL/glu.h>

GLUquadric* quadric;

void reshape(int w, int h) {
    screenW = w;
    screenH = h;
    glViewport(0, 0, w, h);
}

#include "shadow_system.h"
#include "sky.h"

void init() {
    initSky();
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

    gShadows.init(4096); // Initialize Shadow Mapping (Increased res to 4096)

    quadric = gluNewQuadric(); gluQuadricNormals(quadric, GLU_SMOOTH); 
    for (int i = 0; i < 256; ++i) {
        keys[i] = false;
        specialKeys[i] = false;
    }
}
