#ifndef GLOBALS_H
#define GLOBALS_H

#include <GL/glut.h>
#include <GL/glu.h>

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
extern int lastInputTime;

// Game state
extern int gameState; // 0 = menu, 1 = game, 2 = controls, 3 = map select
extern int selectedMap; // 0 = default terrain, 1 = city map

// Terrain
extern const float BLOCK_SIZE;

// Game time
extern float gameTime; // in hours, 0-24, cycles every 24

#endif

