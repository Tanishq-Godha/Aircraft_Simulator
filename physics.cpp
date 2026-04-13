#include "physics.h"
#include "globals.h"
#include "terrain.h"
#include "camera.h"
#include "math_utils.h"
#include <GL/glut.h>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace {

const float kSimulationStep = 1.0f / 120.0f;
const float kMaxFrameDt     = 0.05f;
const float kMaxAirSpeed    = 920.0f;
const float kMaxTaxiSpeed   = 300.0f;

const float RWY_X = 0.0f;
const float RWY_Z = 6200.0f;

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
           sinkRate < 90.0f &&
           (flaps > 0.25f || currentSpeed < 200.0f);
}

bool isHardLanding(float sinkRate) {
    return gearAnimation > 0.97f &&
           isLandingAligned() &&
           currentSpeed < 325.0f &&
           sinkRate < 130.0f;
}

// ─── Afterburner ─────────────────────────────────────────────────────────────
void updateAfterburner(float dt) {
    bool canBurn = !isGrounded && !crashed && !engineOut && throttle >= 0.98f;
    if (!canBurn) afterburnerOn = false;
    float target = afterburnerOn ? 1.0f : 0.0f;
    afterburnerIntensity = approach(afterburnerIntensity, target, 4.0f, dt);
}

// ─── Fuel ────────────────────────────────────────────────────────────────────
void updateFuel(float dt) {
    if (engineOut) return;
    if (!isGrounded && throttle > 0.01f) {
        float burnRate = afterburnerOn ? 0.000042f : 0.000014f;
        fuel -= throttle * burnRate * dt;
        if (fuel <= 0.0f) {
            fuel = 0.0f;
            engineOut        = true;
            afterburnerOn    = false;
            afterburnerIntensity = 0.0f;
        }
    }
}

// ─── Throttle ────────────────────────────────────────────────────────────────
void updateThrottle(float dt) {
    if (engineOut) {
        throttle = approach(throttle, 0.0f, 3.0f, dt);
        return;
    }
    // Faster throttle response (1.8 vs. old 0.5)
    if (keys['r'] || specialKeys[GLUT_KEY_UP])   throttle += 1.8f * dt;
    if (keys['f'] || specialKeys[GLUT_KEY_DOWN])  throttle -= 1.8f * dt;
    throttle = clampf(throttle, 0.0f, 1.0f);
}

// ─── Landing Gear ─────────────────────────────────────────────────────────────
void updateLandingGear(float dt) {
    float gearTarget = gearDeployed ? 1.0f : 0.0f;
    bool  gearMoving = std::fabs(gearAnimation - gearTarget) > 0.01f;

    gearDoorAnim = approach(gearDoorAnim, gearMoving ? 1.0f : 0.0f, 8.0f, dt);
    if (gearDoorAnim > 0.55f || !gearMoving)
        gearAnimation = approach(gearAnimation, gearTarget, 4.0f, dt);
    if (std::fabs(gearAnimation - gearTarget) < 0.01f)
        gearAnimation = gearTarget;

    gearMoving = std::fabs(gearAnimation - gearTarget) > 0.02f;
    if (!gearMoving) {
        gearDoorAnim = approach(gearDoorAnim, 0.0f, 8.0f, dt);
        if (gearDoorAnim < 0.01f) gearDoorAnim = 0.0f;
    }
    gearInTransition = gearMoving || gearDoorAnim > 0.02f;
}

// ─── Taxi physics ────────────────────────────────────────────────────────────
void updateTaxiPhysics(float dt) {
    bool  onRoad = isRoad(planeX, planeZ) || isRunway(planeX, planeZ);
    float surfaceFriction = onRoad ? 1.0f : 0.65f;

    float steerRate = (32.0f - (currentSpeed * 0.05f)) * (onRoad ? 1.0f : 0.7f);
    if (steerRate < 14.0f) steerRate = 14.0f;

    if (keys['q']) yaw += steerRate * dt;
    if (keys['e']) yaw -= steerRate * dt;

    float pitchTarget = 0.0f;
    if (keys['w'])      pitchTarget =  10.0f;
    else if (keys['s']) pitchTarget = -4.0f;

    pitch = approach(pitch, pitchTarget, 4.0f, dt);
    roll  = approach(roll,  0.0f,        9.0f, dt);

    float brakeDrag  = (keys[' '] ? 210.0f : 35.0f) * (onRoad ? 1.0f : 1.5f);

    // Faster acceleration on ground: rate 2.8→5.0
    float targetSpeed = throttle * kMaxTaxiSpeed * surfaceFriction;
    currentSpeed = approach(currentSpeed, targetSpeed, 5.0f, dt);
    currentSpeed -= brakeDrag * dt;
    currentSpeed  = clampf(currentSpeed, 0.0f, kMaxTaxiSpeed);

    wheelRotation += currentSpeed * dt * 5.0f;

    float yRad = degToRad(yaw);
    vX = std::sin(yRad)  * currentSpeed;
    vY = 0.0f;
    vZ = -std::cos(yRad) * currentSpeed;

    // ── Smooth liftoff ────────────────────────────────────────────────────
    // Lower rotation speed 155 (vs. old 195) = shorter takeoff roll.
    // Removed instant planeY jump; vY ramps via approach() = smooth liftoff.
    float rotateSpeed = 155.0f - (flaps * 40.0f);
    if (gearAnimation > 0.97f && currentSpeed > rotateSpeed && pitch > 6.0f) {
        isGrounded = false;
        vY = approach(vY, 55.0f + (flaps * 20.0f), 10.0f, dt);
    }
}

// ─── Air physics ─────────────────────────────────────────────────────────────
void updateAirPhysics(float dt, float agl) {
    bool apActive = autopilotOn || autoLandOn;

    float targetRoll = 0.0f;
    float pitchInput = 0.0f;

    if (!apActive) {
        if (keys['a'])      targetRoll = -45.0f;
        else if (keys['d']) targetRoll =  45.0f;

        if (keys['w']) pitchInput =  30.0f;
        if (keys['s']) pitchInput = -30.0f;

        pitch += pitchInput * dt;

        if (!keys['w'] && !keys['s'])
            pitch = approach(pitch, 0.0f, 1.5f, dt);

        if (keys['q']) yaw += 30.0f * dt;
        if (keys['e']) yaw -= 30.0f * dt;
    }

    roll = approach(roll, targetRoll, 3.5f, dt);
    yaw += (roll / 45.0f) * 0.8f * 30.0f * dt;

    // ── Altitude-hold autopilot (H key) ──────────────────────────────────
    if (autopilotOn && !autoLandOn) {
        float altErr  = autopilotAlt - planeY;
        float tgtPitch = clampf(altErr * 0.04f, -12.0f, 15.0f);
        pitch = approach(pitch, tgtPitch, 2.5f, dt);
        roll  = approach(roll,  0.0f,     1.5f, dt);
        if (keys['w'] || keys['s'])
            autopilotOn = false;
    }

    // ── autoLandOn autopilot: level flight for 5-second cinematic phase ──
    if (autoLandOn && autoLandPhase == 0) {
        float altErr  = autopilotAlt - planeY;
        pitch = approach(pitch, clampf(altErr * 0.04f, -8.0f, 12.0f), 2.5f, dt);
        roll  = approach(roll,  0.0f, 1.5f, dt);
        throttle = approach(throttle, 0.60f, 1.0f, dt);
    }

    pitch = clampf(pitch, -22.0f, 28.0f);

    flapLift = flaps * 190.0f;
    flapDrag = flaps * 115.0f;

    float pRad = degToRad(pitch);
    float yRad = degToRad(yaw);

    float fX = std::sin(yRad) * std::cos(pRad);
    float fY = std::sin(pRad);
    float fZ = -std::cos(yRad) * std::cos(pRad);

    float gearDrag = gearAnimation * 42.0f;
    float abBonus  = afterburnerIntensity * 120.0f;

    float targetSpeed = (throttle * kMaxAirSpeed) + abBonus
                      - (fY * 150.0f) - flapDrag - gearDrag;

    currentSpeed = approach(currentSpeed, targetSpeed, 4.4f, dt);
    currentSpeed = clampf(currentSpeed, 80.0f, kMaxAirSpeed + 120.0f);

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

    if (gearAnimation > 0.95f && flaps > 0.25f &&
        agl < 250.0f && !keys['s'] && !apActive)
        pitch = approach(pitch, 8.0f, 2.4f, dt);

    if (flaps > 0.45f && agl < 2500.0f)
        vY += (-45.0f - vY) * 0.02f;

    if (gearAnimation > 0.01f)
        wheelRotation += currentSpeed * dt * 0.35f;
}

// ─── Inline reset (same as pressing P) ───────────────────────────────────────
void doFullReset() {
    crashed      = false;
    isGrounded   = true;
    gearDeployed = true;
    gearInTransition = false;
    gearAnimation    = 1.0f;
    gearDoorAnim     = 0.0f;
    suspension       = 0.0f;
    wheelRotation    = 0.0f;
    engineFanRotation = 0.0f;
    flaps    = 0.0f;
    flapLift = 0.0f;
    flapDrag = 0.0f;

    planeX = RWY_X;
    planeZ = RWY_Z;
    planeY = getVoxelHeight(RWY_X, RWY_Z) + 12.0f;

    currentSpeed = 0.0f;
    throttle = 0.0f;
    pitch = 0.0f; roll = 0.0f; yaw = 0.0f;
    vX = 0.0f; vY = 0.0f; vZ = 0.0f;
    isStalling = false;

    afterburnerOn        = false;
    afterburnerIntensity = 0.0f;
    fuel       = 1.0f;
    engineOut  = false;
    autopilotOn = false;
    autopilotAlt = 0.0f;
}

// ─── Auto-Land cinematic ─────────────────────────────────────────────────────
// Phase 0 : 5-second level autopilot with HUD countdown
// Phase 1 : 1-second fade-to-black
// Phase 2 : reset plane to runway + 1-second fade back (black→clear)
void updateAutoLand(float dt) {
    if (!autoLandOn) return;

    autoLandTimer -= dt;

    if (autoLandPhase == 0) {
        // Capture altitude on first tick
        static bool capturedAlt = false;
        if (!capturedAlt) {
            autopilotAlt = planeY;
            capturedAlt  = true;
        }
        // autopilot is handled in updateAirPhysics while autoLandPhase==0

        if (autoLandTimer <= 0.0f) {
            autoLandPhase = 1;
            autoLandTimer = 1.0f; // 1 second fade-to-black
            capturedAlt   = false;
        }
    }
    else if (autoLandPhase == 1) {
        // Fade to black: screenFade goes 0 → 1
        screenFade = 1.0f - (autoLandTimer / 1.0f);
        screenFade = clampf(screenFade, 0.0f, 1.0f);

        if (autoLandTimer <= 0.0f) {
            // At peak black: do the full reset
            doFullReset();
            autoLandPhase = 2;
            autoLandTimer = 1.2f; // 1.2 seconds fade back from black
            screenFade = 1.0f;    // ensure fully black
        }
    }
    else if (autoLandPhase == 2) {
        // Fade from black: screenFade goes 1 → 0
        screenFade = autoLandTimer / 1.2f;
        screenFade = clampf(screenFade, 0.0f, 1.0f);

        if (autoLandTimer <= 0.0f) {
            screenFade    = 0.0f;
            autoLandOn    = false;
            autoLandPhase = 0;
            autoLandTimer = 0.0f;
        }
    }
}

float getMaxSceneHeightUnderPlane(float px, float pz, float pyaw) {
    float yRad = degToRad(pyaw);
    float fX = std::sin(yRad), fZ = -std::cos(yRad);
    float rX = std::cos(yRad), rZ =  std::sin(yRad);

    float length   = 10.0f;
    float wingspan = 10.0f;

    float hC  = getSceneHeight(px, pz);
    float hN  = getSceneHeight(px + fX*(length*0.5f),   pz + fZ*(length*0.5f));
    float hL  = getSceneHeight(px - rX*(wingspan*0.5f), pz - rZ*(wingspan*0.5f));
    float hR  = getSceneHeight(px + rX*(wingspan*0.5f), pz + rZ*(wingspan*0.5f));
    float hFL = getSceneHeight(px + fX*15.0f - rX*10.0f, pz + fZ*15.0f - rZ*10.0f);
    float hFR = getSceneHeight(px + fX*15.0f + rX*10.0f, pz + fZ*15.0f + rZ*10.0f);
    float hBL = getSceneHeight(px - fX*15.0f - rX*10.0f, pz - fZ*15.0f - rZ*10.0f);
    float hBR = getSceneHeight(px - fX*15.0f + rX*10.0f, pz - fZ*15.0f + rZ*10.0f);

    float maxH = hC;
    if (hN  > maxH) maxH = hN;
    if (hL  > maxH) maxH = hL;
    if (hR  > maxH) maxH = hR;
    if (hFL > maxH) maxH = hFL;
    if (hFR > maxH) maxH = hFR;
    if (hBL > maxH) maxH = hBL;
    if (hBR > maxH) maxH = hBR;

    float tolerance      = 12.0f;
    float adjustedHeight = maxH - tolerance;
    float baseGround     = getVoxelHeight(px, pz);
    return (adjustedHeight < baseGround) ? baseGround : adjustedHeight;
}

void simulatePhysics(float dt) {
    if (crashed) return;

    // While auto-land is in phases 1 or 2 (cinematic), skip physics
    if (autoLandOn && autoLandPhase >= 1) return;

    updateAfterburner(dt);
    updateFuel(dt);
    updateThrottle(dt);

    if (isGrounded) gearDeployed = true;
    updateLandingGear(dt);

    float currentGround = getMaxSceneHeightUnderPlane(planeX, planeZ, yaw);
    float agl = planeY - currentGround;
    if (agl < 0.0f) agl = 0.0f;

    if (isGrounded) updateTaxiPhysics(dt);
    else            updateAirPhysics(dt, agl);

    if (isGrounded || crashed) {
        autopilotOn       = false;
        afterburnerOn     = false;
    }

    planeX += vX * dt;
    planeY += vY * dt;
    planeZ += vZ * dt;

    float newGround = getMaxSceneHeightUnderPlane(planeX, planeZ, yaw);
    float clearance = getGearClearance();

    if (!isGrounded && newGround > planeY) {
        if (!autoLandOn) {
            crashed = true;
            currentSpeed = 0.0f;
            vX = vY = vZ = 0.0f;
        }
        return;
    }

    if (!isGrounded && planeY <= newGround + clearance) {
        float sinkRate = -vY;
        if (sinkRate < 0.0f) sinkRate = 0.0f;

        float baseGround    = getVoxelHeight(planeX, planeZ);
        bool  onValidRunway = (newGround - baseGround < 20.0f);

        bool safeTouchdown = isSafeLanding(sinkRate) && onValidRunway;
        bool hardTouchdown = isHardLanding(sinkRate) && onValidRunway;

        if (safeTouchdown || hardTouchdown) {
            isGrounded = true;
            planeY = newGround + clearance;
            vY     = 0.0f;
            roll   = hardTouchdown ? 2.0f : 1.0f;
            suspension = hardTouchdown
                       ? clampf(sinkRate * 0.006f,  0.18f, 0.60f)
                       : clampf(sinkRate * 0.0045f, 0.08f, 0.35f);
            currentSpeed *= hardTouchdown ? 0.88f : 0.94f;
        } else {
            crashed = true;
            planeY  = newGround + clearance;
            currentSpeed = 0.0f;
            vX = vY = vZ = 0.0f;
        }
    }

    if (isGrounded) {
        float targetPlaneY = newGround + clearance;
        if (planeY > targetPlaneY + 2.0f) {
            isGrounded = false;
        } else {
            planeY     = targetPlaneY;
            isStalling = false;
        }
    }

    engineFanRotation += (180.0f + throttle * 3200.0f - currentSpeed * 0.4f) * dt;
    if (engineFanRotation > 360.0f)
        engineFanRotation = std::fmod(engineFanRotation, 360.0f);

    updateCamera(dt);
    suspension = approach(suspension, 0.0f, isGrounded ? 2.5f : 6.0f, dt);
}

void updateLightTimer(float dt) {
    lightTimer += dt;
    if (lightTimer > 10000.0f)
        lightTimer = std::fmod(lightTimer, 10000.0f);
}

} // namespace

void updatePhysics() {
    static float accumulator = 0.0f;

    if (isPaused) {
        glutPostRedisplay();
        return;
    }

    gameTime += deltaTime * (12.0f / 300.0f) * timeScale;
    if (gameTime >= 24.0f) gameTime -= 24.0f;

    // Tick the auto-land cinematic timer
    if (autoLandOn) updateAutoLand(deltaTime);

    accumulator += deltaTime;
    while (accumulator >= kSimulationStep) {
        simulatePhysics(kSimulationStep);
        accumulator -= kSimulationStep;
    }

    updateLightTimer(deltaTime);
    glutPostRedisplay();
}
