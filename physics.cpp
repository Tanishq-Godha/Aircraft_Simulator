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

float getMaxSceneHeightUnderPlane(float px, float pz, float pyaw);

const float kSimulationStep = 1.0f / 120.0f;
const float kMaxFrameDt     = 0.05f;
const float kMaxAirSpeed    = 920.0f;
const float kMaxTaxiSpeed   = 300.0f;
const float kGravity        = 180.0f;  // Game units/s² (tuned for game scale)

const float RWY_X = 0.0f;
const float RWY_Z = 6200.0f;

float approach(float current, float target, float rate, float dt) {
    float alpha = clampf(rate * dt, 0.0f, 1.0f);
    return current + (target - current) * alpha;
}

float getGearClearance() {
    if (isBellyLanding) return 2.0f;
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
           currentSpeed < 240.0f &&
           sinkRate < 90.0f;
}

bool isHardLanding(float sinkRate) {
    return gearAnimation > 0.97f &&
           isLandingAligned() &&
           currentSpeed < 325.0f &&
           sinkRate < 130.0f;
}

bool isBellyLandingCheck(float sinkRate) {
    return gearAnimation < 0.05f &&
           isLandingAligned() &&
           currentSpeed <= 280.0f &&
           sinkRate < 90.0f;
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
    float speedRate = autoLandOn ? 0.3f : 5.0f; // Smooth braking for autoland
    currentSpeed = approach(currentSpeed, targetSpeed, speedRate, dt);
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
    float rotateSpeed = 155.0f;
    if (gearAnimation > 0.97f && currentSpeed > rotateSpeed && pitch > 6.0f) {
        isGrounded = false;
        vY = approach(vY, 65.0f, 10.0f, dt);
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

    // Allow steeper dives for full stall simulation, but clamp up-pitch
    pitch = clampf(pitch, -85.0f, 40.0f);

    float pRad = degToRad(pitch);
    float yRad = degToRad(yaw);

    float fX = std::sin(yRad) * std::cos(pRad);
    float fY = std::sin(pRad);
    float fZ = -std::cos(yRad) * std::cos(pRad);

    float gearDrag = gearAnimation * 42.0f;
    float abBonus  = afterburnerIntensity * 120.0f;

    // Gravity massively dictates speed when the engine is low/off (like a real glider)
    float gravitySpeedImpact = -fY * 600.0f;
    
    // Terminal velocity naturally limits how fast the plane can fall/dive
    float targetSpeed = (throttle * kMaxAirSpeed) + abBonus + gravitySpeedImpact - gearDrag;

    // Gravity accelerates you down fast, but air friction decelerates you slowly
    float accelRate = (targetSpeed > currentSpeed) ? 2.5f : 0.45f;
    currentSpeed = approach(currentSpeed, targetSpeed, accelRate, dt);
    currentSpeed = clampf(currentSpeed, 10.0f, kMaxAirSpeed + 250.0f); // Allow speed leaps in a steep dive

    float stallSpeed = 160.0f + (gearAnimation * 10.0f);
    isStalling = currentSpeed < stallSpeed && pitch > 10.0f;

    // True aerodynamic lift vs gravity
    vX = fX * currentSpeed;
    vZ = fZ * currentSpeed;

    // Base intended vertical speed from the nose vector
    float target_vY = fY * currentSpeed;

    // Approach the nose vector's velocity (simulates some inertia)
    vY = approach(vY, target_vY, 3.5f, dt);

    // Lift generation requires speed and correct pitch (simplified)
    float liftFactor = clampf((currentSpeed - stallSpeed + 50.0f) / 150.0f, 0.0f, 1.0f); 
    // High bank angles (roll) bleed off vertical lift
    float rollFactor = std::cos(degToRad(roll));
    float upLift = kGravity * liftFactor * rollFactor;

    // Apply net gravity force (if flying fast and level, this is 0)
    vY -= (kGravity - upLift) * dt;

    if (isStalling) {
        // Drop more aggressively if the wings fully stall from high pitch
        vY -= kGravity * 1.5f * dt;
        if (!autoLandOn) {
            pitch = approach(pitch, -15.0f, 1.2f, dt); // Nose drops down naturally
        }
    } else if (currentSpeed < stallSpeed && !autoLandOn) {
        // If simply slow and not auto-landing, gentle nose drop
        pitch = approach(pitch, -5.0f, 1.0f, dt);
    }

    // Turbulence / subtle sway
    vX += std::sin(gameTime * 0.8f) * 1.5f;
    vZ += std::cos(gameTime * 0.9f) * 1.5f;

    if (gearAnimation > 0.01f)
        wheelRotation += currentSpeed * dt * 0.35f;
}

// ─── Auto-Land rapid deceleration ──────────────────────────────────────────────
void updateAutoLand(float dt) {
    if (!autoLandOn) return;

    if (!isGrounded) {
        float agl = planeY - getMaxSceneHeightUnderPlane(planeX, planeZ, yaw);
        
        float stallSpeed = 160.0f + (gearAnimation * 10.0f);
        float approachSpeed = stallSpeed + 25.0f; // Safe margin above stall

        roll = approach(roll, 0.0f, 1.5f, dt);
        
        if (agl > 40.0f) {
            // Glide slope: point nose slightly down
            pitch = approach(pitch, -4.0f, 1.0f, dt);
            
            // Autothrottle: actively manage throttle to maintain approach speed 
            if (currentSpeed < approachSpeed) {
                throttle = approach(throttle, 0.5f, 2.0f, dt);
            } else {
                throttle = approach(throttle, 0.0f, 2.0f, dt);
            }
        } else {
            // Flare: near the ground, pitch up and kill throttle to gently stall onto runway
            pitch = approach(pitch, 6.0f, 1.5f, dt);
            throttle = approach(throttle, 0.0f, 2.0f, dt);
        }
    } else {
        // On the ground: brake to a stop smoothly
        pitch = approach(pitch, 0.0f, 2.0f, dt);
        throttle = 0.0f;
        if (currentSpeed <= 1.0f) {
            currentSpeed = 0.0f;
            autoLandOn = false; // Deceleration complete
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

    updateAfterburner(dt);
    updateFuel(dt);
    updateThrottle(dt);

    if (isGrounded) gearDeployed = true;
    updateLandingGear(dt);

    if (autoLandFailTimer > 0.0f) {
        autoLandFailTimer -= dt;
    }

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

if (!isExploding && !crashed && !isGrounded && newGround > planeY) {
        if (!autoLandOn) {
            isExploding = true;
            explosionTimer = 0.0f;
            currentSpeed *= 0.5f;
        }
        return;
    }

    if (!isExploding && !crashed && !isGrounded && planeY <= newGround + clearance) {
        float sinkRate = -vY;
        if (sinkRate < 0.0f) sinkRate = 0.0f;

        float baseGround    = getVoxelHeight(planeX, planeZ);
        bool  onValidRunway = (newGround - baseGround < 20.0f);

        bool safeTouchdown = isSafeLanding(sinkRate) && onValidRunway;
        bool hardTouchdown = isHardLanding(sinkRate) && onValidRunway;
        bool bellyTouchdown = isBellyLandingCheck(sinkRate) && onValidRunway;

        if (safeTouchdown || hardTouchdown) {
            isGrounded = true;
            planeY = newGround + clearance;
            vY     = 0.0f;
            roll   = hardTouchdown ? 2.0f : 1.0f;
            suspension = hardTouchdown
                       ? clampf(sinkRate * 0.006f,  0.18f, 0.60f)
                       : clampf(sinkRate * 0.0045f, 0.08f, 0.35f);
            currentSpeed *= hardTouchdown ? 0.88f : 0.94f;
        } else if (bellyTouchdown) {
            isGrounded = true;
            isBellyLanding = true;
            planeY = newGround + clearance;
            vY = 0.0f;
            currentSpeed *= 0.70f;
        } else {
            isExploding = true;
            explosionTimer = 0.0f;
            planeY  = newGround + clearance;
            currentSpeed *= 0.5f;
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
        
        if (isBellyLanding) {
            targetPlaneY += 0.5f;
            currentSpeed -= 150.0f * dt; // Heavy friction
            if (currentSpeed <= 5.0f) {
                currentSpeed = 0.0f;
                isExploding = true;
                explosionTimer = 0.0f;
            }
        }
    }

    if (isExploding) {
        explosionTimer += dt;
        currentSpeed = approach(currentSpeed, 0.0f, 150.0f, dt);
        if (explosionTimer > 0.4f) {
            crashed = true;
            isExploding = false;
            currentSpeed = 0.0f;
            vX = vY = vZ = 0.0f;
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
