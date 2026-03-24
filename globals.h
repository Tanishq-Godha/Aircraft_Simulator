#ifndef GLOBALS_H
#define GLOBALS_H

#include <GL/glut.h>

// Window
extern int screenW, screenH;
extern bool keys[256];

// Flight state
extern float planeX, planeY, planeZ;
extern float pitch, roll, yaw;
extern float vX, vY, vZ;
extern float throttle;
extern float currentSpeed;
extern bool isStalling;

// Camera
extern int cameraMode;

// Terrain
extern const float BLOCK_SIZE;

// GL resources
extern GLUquadric* quadric;

#endif