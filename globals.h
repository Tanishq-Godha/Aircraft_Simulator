#ifndef GLOBALS_H
#define GLOBALS_H

#include <GL/glut.h>

// Window
extern int screenW, screenH;
extern bool keys[256];
extern bool specialKeys[256];

// Flight state
extern float planeX, planeY, planeZ;
extern float pitch, roll, yaw;
extern float vX, vY, vZ;
extern float throttle;
extern float currentSpeed;
extern bool isStalling;
extern bool gearDeployed;
extern bool gearInTransition;
extern bool isGrounded;
extern bool crashed;

// Landing system
extern float gearAnimation;
extern float gearDoorAnim;
extern float suspension;
extern float wheelRotation;
extern float engineFanRotation;

// Flaps
extern float flaps;
extern float flapLift;
extern float flapDrag;

// Camera
extern int cameraMode;

// Terrain
extern const float BLOCK_SIZE;

// GL resources
extern GLUquadric* quadric;

#endif

