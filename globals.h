#ifndef GLOBALS_H
#define GLOBALS_H

#include <GL/glut.h>
#include <GL/glu.h>

// Window
extern int screenW, screenH;
extern bool keys[256];
extern bool specialKeys[256];
extern float deltaTime; // Global frame time

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
extern bool isPaused;

// Terrain
extern const float BLOCK_SIZE;

// Game time
extern float gameTime; // in hours, 0-24, cycles every 24

// Lighting
extern float lightTimer;
extern float timeScale;

// Afterburner
extern bool  afterburnerOn;
extern float afterburnerIntensity; // 0..1 animated

// Fuel
extern float fuel;       // 1.0 = full, 0.0 = empty
extern bool  engineOut;

// Autopilot (H key)
extern bool  autopilotOn;
extern float autopilotAlt; // target altitude when AP engaged

// Auto-Land (L key)
extern bool autoLandOn;
extern int  autoLandPhase; // 0=autopilot countdown, 1=fade-to-black, 2=reset+fade-in
extern float autoLandTimer; // seconds remaining in current phase

// Screen fade (0=normal, 1=full black) — used for landing cinematic
extern float screenFade;

// Weather control (0=Dynamic, 1=Clear, 2=Cloudy, 3=Foggy)
extern int weatherMode;

// Textures
extern GLuint cloudTextureId;

#endif

