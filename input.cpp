#include "input.h"
#include "globals.h"
#include "terrain.h"
#include <cctype>
#include <cstdlib>
#include <GL/glut.h>

void keyDown(unsigned char key, int, int) {
    lastInputTime = glutGet(GLUT_ELAPSED_TIME);

    if (gameState == 0) { // Menu
        if (key == '1') gameState = 1;
        else if (key == '2') gameState = 2;
        else if (key == '3') gameState = 3;
        return;
    } else if (gameState == 2 || gameState == 3) {
        if (key == 'b' || key == 'B') gameState = 0;
        else if (gameState == 3 && key == '1') { selectedMap = 0; gameState = 0; }
        else if (gameState == 3 && key == '2') { selectedMap = 1; gameState = 0; }
        return;
    }

    // Y = Pause / Resume (processed before pause check)
    if (key == 'y' || key == 'Y') {
        isPaused = !isPaused;
        return;
    }

    // Esc → go back to menu
    if (key == 27) {
        isPaused = false;
        gameState = 0;
        return;
    }

    // If paused, ignore all other game inputs
    if (isPaused) return;

    unsigned char lowerKey = static_cast<unsigned char>(std::tolower(key));
    keys[lowerKey] = true;

    if (key == '+' || key == '=') throttle += 0.08f;
    if (key == '-' || key == '_') throttle -= 0.08f;

    if (lowerKey == 'g' && !crashed && !isGrounded) {
        gearDeployed = !gearDeployed;
    }

    if (key == '[') flaps -= 0.2f;
    if (key == ']') flaps += 0.2f;

    if (flaps < 0.0f) flaps = 0.0f;
    if (flaps > 1.0f) flaps = 1.0f;
    if (throttle < 0.0f) throttle = 0.0f;
    if (throttle > 1.0f) throttle = 1.0f;

    // P = Full Reset
    if (lowerKey == 'p') {
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

        planeX = 0.0f;
        planeZ = 6200.0f;
        planeY = getVoxelHeight(planeX, planeZ) + 12.0f;

        currentSpeed = 0.0f;
        throttle = 0.0f;
        pitch = 0.0f;
        roll  = 0.0f;
        yaw   = 0.0f;
        vX = 0.0f;
        vY = 0.0f;
        vZ = 0.0f;
        isStalling = false;

        afterburnerOn        = false;
        afterburnerIntensity  = 0.0f;
        fuel        = 1.0f;
        engineOut   = false;
        autopilotOn  = false;
        autopilotAlt = 0.0f;
        autoLandOn    = false;
        autoLandPhase = 0;
        autoLandTimer = 0.0f;
        screenFade    = 0.0f;
        isPaused      = false;
    }

    if (lowerKey == 'v')
        cameraMode = (cameraMode + 1) % 4;

    if (lowerKey == 'm' && gameState == 1)
        gameState = 0;

    if (lowerKey == 't') {
        if (timeScale < 2.0f)        timeScale = 20.0f;
        else if (timeScale < 25.0f)  timeScale = 100.0f;
        else if (timeScale < 150.0f) timeScale = 500.0f;
        else                         timeScale = 1.0f;
    }

    // H = Autopilot Altitude Hold toggle
    if (lowerKey == 'h' && !crashed && !isGrounded && !autoLandOn) {
        autopilotOn = !autopilotOn;
        if (autopilotOn) {
            autopilotAlt = planeY; // capture current altitude
        }
    }

    // O = Jump Time (Fast forward 6 hours)
    if (lowerKey == 'o' && !isPaused) {
        gameTime += 6.0f;
        if (gameTime >= 24.0f) gameTime -= 24.0f;
    }

    // K = Cycle Weather Mode (Dynamic -> Clear -> Cloudy -> Foggy)
    if (lowerKey == 'k' && gameState == 1) {
        weatherMode = (weatherMode + 1) % 4;
    }

    // L = Auto-Land cinematic (5s autopilot → fade to black → reset to runway)

    if (lowerKey == 'l' && !crashed && !isGrounded) {
        autoLandOn = !autoLandOn;
        if (autoLandOn) {
            autoLandPhase = 0;
            autoLandTimer = 5.0f;  // 5-second autopilot phase
            autopilotOn   = false;
            gearDeployed  = true;
        } else {
            autoLandPhase = 0;
            autoLandTimer = 0.0f;
            screenFade    = 0.0f;
        }
    }
}

void keyUp(unsigned char key, int, int) {
    keys[static_cast<unsigned char>(std::tolower(key))] = false;
}

void specialKeyDown(int key, int, int) {
    lastInputTime = glutGet(GLUT_ELAPSED_TIME);
    if (!isPaused && key >= 0 && key < 256) {
        specialKeys[key] = true;
    }
}

void specialKeyUp(int key, int, int) {
    if (key >= 0 && key < 256) {
        specialKeys[key] = false;
    }
}
