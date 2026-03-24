#include "input.h"
#include "globals.h"
#include "terrain.h"
#include <cctype>
#include <cstdlib>

void keyDown(unsigned char key, int, int) {

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

    if (lowerKey == 'p') {
        crashed = false;
        isGrounded = false;
        gearDeployed = false;
        gearInTransition = false;
        gearAnimation = 0.0f;
        gearDoorAnim = 0.0f;
        suspension = 0.0f;
        wheelRotation = 0.0f;
        engineFanRotation = 0.0f;
        flaps = 0.0f;
        flapLift = 0.0f;
        flapDrag = 0.0f;
        planeY = getVoxelHeight(planeX, planeZ) + 2000.0f;
        currentSpeed = 400.0f;
        throttle = 0.8f;
        pitch = 0.0f;
        roll = 0.0f;
        yaw = 0.0f;
        vX = 0.0f;
        vY = 0.0f;
        vZ = 400.0f;
        isStalling = false;
    }

    if (lowerKey == 'v')
        cameraMode = (cameraMode + 1) % 4;

    if (key == 27) exit(0);
}

void keyUp(unsigned char key, int, int) {
    keys[static_cast<unsigned char>(std::tolower(key))] = false;
}

void specialKeyDown(int key, int, int) {
    if (key >= 0 && key < 256) {
        specialKeys[key] = true;
    }
}

void specialKeyUp(int key, int, int) {
    if (key >= 0 && key < 256) {
        specialKeys[key] = false;
    }
}

