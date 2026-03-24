#include "globals.h"

int screenW = 1024, screenH = 768;
bool keys[256];

float planeX = 0.0f, planeY = 5000.0f, planeZ = 0.0f;
float pitch = 0.0f, roll = 0.0f, yaw = 0.0f;
float vX = 0.0f, vY = 0.0f, vZ = 400.0f;

float throttle = 0.8f;
float currentSpeed = 400.0f;
bool isStalling = false;

int cameraMode = 0;

const float BLOCK_SIZE = 150.0f;

GLUquadric* quadric = nullptr;