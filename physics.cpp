#include "physics.h"
#include "globals.h"
#include "terrain.h"
#include "math_utils.h"
#include <GL/glut.h>
#include <cmath>

namespace {

const float kSimulationStep = 1.0f / 120.0f;
const float kMaxFrameDt = 0.05f;
const float kMaxAirSpeed = 920.0f;
const float kMaxTaxiSpeed = 300.0f;

float clampf(float value, float minValue, float maxValue) {
    if (value < minValue) return minValue;
    if (value > maxValue) return maxValue;
    return value;
}

float approach(float current, float target, float rate, float dt) {
    float alpha = clampf(rate * dt, 0.0f, 1.0f);
    return current + (target - current) * alpha;
}

float getGearClearance() {
    return lerp(2.0f, 12.0f, gearAnimation);
}

bool isLandingAligned() {
    return std::fabs(roll) < 10.0f &&
           pitch > -10.0f &&
           pitch < 16.0f;
}

bool isSafeLanding(float sinkRate) {
    return gearAnimation > 0.97f &&
           isLandingAligned() &&
           currentSpeed < 285.0f &&
           sinkRate < 90.0f;
}

bool isHardLanding(float sinkRate) {
    return gearAnimation > 0.97f &&
           isLandingAligned() &&
           currentSpeed < 325.0f &&
           sinkRate < 130.0f;
}

void updateThrottle(float dt) {
    if (keys['r'] || specialKeys[GLUT_KEY_UP]) throttle += 0.5f * dt;
    if (keys['f'] || specialKeys[GLUT_KEY_DOWN]) throttle -= 0.5f * dt;
    throttle = clampf(throttle, 0.0f, 1.0f);
}



void updateLandingGear(float dt) {
    float gearTarget = gearDeployed ? 1.0f : 0.0f;
    bool gearMoving = std::fabs(gearAnimation - gearTarget) > 0.01f;

    gearDoorAnim = approach(gearDoorAnim, gearMoving ? 1.0f : 0.0f, 8.0f, dt);

    if (gearDoorAnim > 0.55f || !gearMoving) {
        gearAnimation = approach(gearAnimation, gearTarget, 4.0f, dt);
    }

    if (std::fabs(gearAnimation - gearTarget) < 0.01f) {
        gearAnimation = gearTarget;
    }

    gearMoving = std::fabs(gearAnimation - gearTarget) > 0.02f;

    if (!gearMoving) {
        gearDoorAnim = approach(gearDoorAnim, 0.0f, 8.0f, dt);
        if (gearDoorAnim < 0.01f) {
            gearDoorAnim = 0.0f;
        }
    }

    gearInTransition = gearMoving || gearDoorAnim > 0.02f;
}

void updateTaxiPhysics(float dt) {
    float steerRate = 32.0f - (currentSpeed * 0.05f);
    if (steerRate < 14.0f) steerRate = 14.0f;

    if (keys['q']) yaw += steerRate * dt;
    if (keys['e']) yaw -= steerRate * dt;

    float pitchTarget = 0.0f;
    if (keys['w']) pitchTarget = 10.0f;
    else if (keys['s']) pitchTarget = -4.0f;

    pitch = approach(pitch, pitchTarget, 4.0f, dt);
    roll = approach(roll, 0.0f, 9.0f, dt);

    float brakeDrag = keys[' '] ? 210.0f : 35.0f;
    float targetSpeed = throttle * kMaxTaxiSpeed;
    currentSpeed = approach(currentSpeed, targetSpeed, 2.8f, dt);
    currentSpeed -= brakeDrag * dt;
    currentSpeed = clampf(currentSpeed, 0.0f, kMaxTaxiSpeed);

    wheelRotation += currentSpeed * dt * 5.0f;

    float yRad = degToRad(yaw);
    vX = std::sin(yRad) * currentSpeed;
    vY = 0.0f;
    vZ = -std::cos(yRad) * currentSpeed;

    float rotateSpeed = 190.0f - (flaps * 45.0f);
    if (gearAnimation > 0.97f && currentSpeed > rotateSpeed && pitch > 5.5f) {
        isGrounded = false;
        planeY += 5.0f;
        vY = 60.0f + (flaps * 20.0f);
        suspension = 0.0f;
    }
}

void updateAirPhysics(float dt, float agl) {
    float targetRoll = 0.0f;
    float pitchInput = 0.0f;

    // Roll controls (aileron) for realistic banking
    if (keys['a']) targetRoll = -45.0f;
    else if (keys['d']) targetRoll = 45.0f;

    roll = approach(roll, targetRoll, 3.5f, dt);

    // Pitch controls (elevator)
    if (keys['w']) pitchInput = 30.0f;
    if (keys['s']) pitchInput = -30.0f;

    pitch += pitchInput * dt;

    // Return toward level pitch gradually when no input
    if (!keys['w'] && !keys['s']) {
        pitch = approach(pitch, 0.0f, 1.5f, dt);
    }

    // Rudder controls (q/e)
    if (keys['q']) yaw += 30.0f * dt;
    if (keys['e']) yaw -= 30.0f * dt;

    // Dynamic yaw from banked turn in aileron mode
    float bankTurnFactor = 0.8f; // strafing effect from roll
    yaw += (roll / 45.0f) * bankTurnFactor * 30.0f * dt;

    pitch = clampf(pitch, -22.0f, 28.0f);

    flapLift = flaps * 190.0f;
    flapDrag = flaps * 115.0f;

    float pRad = degToRad(pitch);
    float yRad = degToRad(yaw);

    float fX = std::sin(yRad) * std::cos(pRad);
    float fY = std::sin(pRad);
    float fZ = -std::cos(yRad) * std::cos(pRad);

    float gearDrag = gearAnimation * 42.0f;
    float targetSpeed = (throttle * kMaxAirSpeed) -
                        (fY * 150.0f) -
                        flapDrag -
                        gearDrag;

    currentSpeed = approach(currentSpeed, targetSpeed, 4.4f, dt);
    currentSpeed = clampf(currentSpeed, 80.0f, kMaxAirSpeed);

    float stallSpeed = 145.0f - (flaps * 30.0f) + (gearAnimation * 10.0f);
    isStalling = currentSpeed < stallSpeed && pitch > 10.0f;

    vX = fX * currentSpeed;
    vY = fY * currentSpeed + (flapLift * dt);
    vZ = fZ * currentSpeed;

    if (isStalling) {
        vY -= 90.0f * dt;
        roll = approach(roll, 0.0f, 1.2f, dt);
        currentSpeed = approach(currentSpeed, stallSpeed - 10.0f, 2.0f, dt);
    }

    if (gearAnimation > 0.95f &&
        flaps > 0.25f &&
        agl < 250.0f &&
        !keys['s']) {
        pitch = approach(pitch, 8.0f, 2.4f, dt);
    }

    if (flaps > 0.45f && agl < 2500.0f) {
        vY += (-45.0f - vY) * 0.02f;
    }

    if (gearAnimation > 0.01f) {
        wheelRotation += currentSpeed * dt * 0.35f;
    }
}

float getMaxSceneHeightUnderPlane(float px, float pz, float pyaw) {
    float yRad = degToRad(pyaw);
    float fX = std::sin(yRad), fZ = -std::cos(yRad);
    float rX = std::cos(yRad), rZ = std::sin(yRad);

    float length = 10.0f;   // Was 45.0f
float wingspan = 10.0f; // Was 54.0f

    // Point sampling: Center, Nose, Left/Right Wingtips, and mid-fuselage corners
    float hC = getSceneHeight(px, pz);
    float hN = getSceneHeight(px + fX * (length * 0.5f), pz + fZ * (length * 0.5f));
    float hL = getSceneHeight(px - rX * (wingspan * 0.5f), pz - rZ * (wingspan * 0.5f));
    float hR = getSceneHeight(px + rX * (wingspan * 0.5f), pz + rZ * (wingspan * 0.5f));
    
    // Add fuselage corners for better edge detection
    float hFL = getSceneHeight(px + fX * 15.0f - rX * 10.0f, pz + fZ * 15.0f - rZ * 10.0f);
    float hFR = getSceneHeight(px + fX * 15.0f + rX * 10.0f, pz + fZ * 15.0f + rZ * 10.0f);
    float hBL = getSceneHeight(px - fX * 15.0f - rX * 10.0f, pz - fZ * 15.0f - rZ * 10.0f);
    float hBR = getSceneHeight(px - fX * 15.0f + rX * 10.0f, pz - fZ * 15.0f + rZ * 10.0f);

    float maxH = hC;
    if (hN > maxH) maxH = hN;
    if (hL > maxH) maxH = hL;
    if (hR > maxH) maxH = hR;
    if (hFL > maxH) maxH = hFL;
    if (hFR > maxH) maxH = hFR;
    if (hBL > maxH) maxH = hBL;
    if (hBR > maxH) maxH = hBR;

    float tolerance = 12.0f; 
    float adjustedHeight = maxH - tolerance;
    float baseGround = getVoxelHeight(px, pz);
    return (adjustedHeight < baseGround) ? baseGround : adjustedHeight;
}

void simulatePhysics(float dt) {
    if (crashed) {
        return;
    }

    updateThrottle(dt);

    if (isGrounded) {
        gearDeployed = true;
    }

    updateLandingGear(dt);

    // Get the highest point under the plane's wings/nose (with tolerance)
    float currentGround = getMaxSceneHeightUnderPlane(planeX, planeZ, yaw);
    float agl = planeY - currentGround;
    if (agl < 0.0f) {
        agl = 0.0f;
    }

    if (isGrounded) updateTaxiPhysics(dt);
    else updateAirPhysics(dt, agl);

    // Store previous Y altitude to calculate horizontal wall crashes
    // Store previous Y altitude
    float prevY = planeY;

    planeX += vX * dt;
    planeY += vY * dt;
    planeZ += vZ * dt;
    

    // Get the new ground/obstacle height after moving
    float newGround = getMaxSceneHeightUnderPlane(planeX, planeZ, yaw);
    float clearance = getGearClearance();

    // --- HORIZONTAL WALL COLLISION (FIXED) ---
    // If the terrain/building height is suddenly taller than our actual flying altitude, we hit a wall!
    if (!isGrounded && newGround > planeY) {
        crashed = true;
        currentSpeed = 0.0f;
        vX = 0.0f; vY = 0.0f; vZ = 0.0f;
        return;
    }

    // --- VERTICAL COLLISION / LANDING ---
    if (!isGrounded && planeY <= newGround + clearance) {
        float sinkRate = -vY;
        if (sinkRate < 0.0f) {
            sinkRate = 0.0f;
        }

        // Must be landing on actual terrain/road, not the roof of a skyscraper
        float baseGround = getVoxelHeight(planeX, planeZ);
        bool onValidRunway = (newGround - baseGround < 20.0f); 

        bool safeTouchdown = isSafeLanding(sinkRate) && onValidRunway;
        bool hardTouchdown = isHardLanding(sinkRate) && onValidRunway;

        if (safeTouchdown || hardTouchdown) {
            isGrounded = true;
            planeY = newGround + clearance;
            vY = 0.0f;
            roll = hardTouchdown ? 2.0f : 1.0f;
            suspension = hardTouchdown
                       ? clampf(sinkRate * 0.006f, 0.18f, 0.60f)
                       : clampf(sinkRate * 0.0045f, 0.08f, 0.35f);
            currentSpeed *= hardTouchdown ? 0.88f : 0.94f;
        } else {
            // Crash! (Landed too hard or landed on a building roof)
            crashed = true;
            planeY = newGround + clearance; 
            currentSpeed = 0.0f;
            vX = 0.0f; vY = 0.0f; vZ = 0.0f;
        }
    }

    // --- GROUND CLAMPING & SAFETY ---
    if (planeY < newGround + clearance) {
        planeY = newGround + clearance;
        if (vY < -10.0f && !isGrounded) {
             // We hit the ground but didn't land safe (crash logic already handled above)
        } else if (vY < 0.0f) {
            vY = 0.0f; // Prevent digging into ground
        }
    }

    if (isGrounded) {
        float targetPlaneY = newGround + clearance;
        if (planeY > targetPlaneY + 5.0f) { // Increased threshold slightly for hills
            isGrounded = false; // Drove off a cliff or ledge
        } else {
            planeY = targetPlaneY;
            isStalling = false;
        }
    }

    engineFanRotation += (180.0f + throttle * 3200.0f - currentSpeed * 0.4f) * dt;
    if (engineFanRotation > 360.0f) {
        engineFanRotation = std::fmod(engineFanRotation, 360.0f);
    }

    suspension = approach(suspension, 0.0f, isGrounded ? 2.5f : 6.0f, dt);
}


void updateLightTimer(float dt) {
    lightTimer += dt;
    if (lightTimer > 10000.0f) lightTimer = std::fmod(lightTimer, 10000.0f);
}

} // namespace

void updatePhysics() {
    static int lastTickMs = 0;
    static float accumulator = 0.0f;

    int nowMs = glutGet(GLUT_ELAPSED_TIME);
    if (lastTickMs == 0) {
        lastTickMs = nowMs;
        glutPostRedisplay();
    }

    float frameDt = (nowMs - lastTickMs) * 0.001f;
    lastTickMs = nowMs;

    // Update game time: 5 min real = 12 hours game (apply timeScale)
    gameTime += frameDt * (12.0f / 300.0f) * timeScale;
    if (gameTime >= 24.0f) gameTime -= 24.0f;

    if (frameDt < 0.0f) {
        frameDt = 0.0f;
    } else if (frameDt > kMaxFrameDt) {
        frameDt = kMaxFrameDt;
    }

    accumulator += frameDt;
    while (accumulator >= kSimulationStep) {
        simulatePhysics(kSimulationStep);
        accumulator -= kSimulationStep;
    }

    updateLightTimer(frameDt);

    glutPostRedisplay();
}
