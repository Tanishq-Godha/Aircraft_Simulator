#include "globals.h"

int screenW = 1024, screenH = 768;
bool keys[256];
bool specialKeys[256];

float planeX = 0.0f, planeY = 412.0f, planeZ = 6200.0f;
float pitch = 0.0f, roll = 0.0f, yaw = 0.0f;
float vX = 0.0f, vY = 0.0f, vZ = 0.0f;

float throttle = 0.0f;
float currentSpeed = 0.0f;
bool isStalling = false;

bool gearDeployed = true;
bool gearInTransition = false;
bool isGrounded = true;
bool crashed = false;

// Landing system
float gearAnimation = 1.0f;
float gearDoorAnim = 0.0f;
float suspension = 0.0f;
float wheelRotation = 0.0f;
float engineFanRotation = 0.0f;

// Flaps
float flaps = 0.0f;
float flapLift = 0.0f;
float flapDrag = 0.0f;

int cameraMode = 0;
int lastInputTime = 0;

int gameState = 0; // Start with menu
int selectedMap = 0;

const float BLOCK_SIZE = 150.0f;

float gameTime = 6.2f; // 06:12 AM
float lightTimer = 0.0f; // for blinking aircraft lights
float timeScale = 1.0f;

