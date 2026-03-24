#include "physics.h"
#include "globals.h"
#include "terrain.h"
#include "math_utils.h"
#include <GL/glut.h>

void updatePhysics() {
    float dt = 0.016f; 

    // 1. Target-Based Controls (Fixes poor left/right feel)
    float targetRoll = 0.0f;
    float pitchInput = 0.0f;

    // A/D keys set a target bank angle rather than infinitely spinning
    if (keys['a']) targetRoll = -75.0f; 
    else if (keys['d']) targetRoll = 75.0f;
    
    // Smoothly interpolate current roll to target roll
    roll += (targetRoll - roll) * 6.0f * dt;

    // Pitch controls
    if (keys['w']) pitchInput = -90.0f; 
    if (keys['s']) pitchInput = 90.0f; 
    pitch += pitchInput * dt;

    // Auto-level pitch smoothly when keys are released
    if (!keys['w'] && !keys['s']) pitch *= 0.92f; 

    // Rudder (Yaw)
    if (keys['q']) yaw += 40.0f * dt; 
    if (keys['e']) yaw -= 40.0f * dt; 

    // Throttle
    if (keys['r']) throttle += 0.5f * dt; 
    if (keys['f']) throttle -= 0.5f * dt; 

    if (throttle > 1.0f) throttle = 1.0f;
    if (throttle < 0.0f) throttle = 0.0f;
    if (pitch > 85.0f) pitch = 85.0f;
    if (pitch < -85.0f) pitch = -85.0f;

    // Bank-to-turn formula (Automatically turns the plane based on how far it is rolled)
    yaw -= (roll * 0.9f) * dt; 

    // Velocity Math
    float pRad = degToRad(pitch); float yRad = degToRad(yaw);
    float fX = sin(yRad) * cos(pRad); float fY = -sin(pRad); float fZ = -cos(yRad) * cos(pRad);

    float maxSpeed = 600.0f;
    float targetSpeed = (throttle * maxSpeed) - (fY * 150.0f); 
    currentSpeed = currentSpeed * 0.95f + targetSpeed * 0.05f; 
    isStalling = (currentSpeed < 100.0f && pitch > 10.0f);

    if (isStalling) {
        vY -= GRAVITY * 0.5f; vX *= 0.99f; vZ *= 0.99f; pitch -= 1.0f; 
    } else {
        vX = fX * currentSpeed; vY = fY * currentSpeed; vZ = fZ * currentSpeed;
    }

    planeX += vX * dt; planeY += vY * dt; planeZ += vZ * dt;

    float blockHeight = getVoxelHeight(planeX, planeZ);
    if (planeY < blockHeight + 5.0f) {
        planeY = blockHeight + 5.0f; 
        currentSpeed *= 0.5f; pitch = 0.0f; roll = 0.0f; 
    }
    glutPostRedisplay();
}