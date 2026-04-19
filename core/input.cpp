#include "input.h"
#include "globals.h"
#include "terrain.h"
#include "menu.h"
#include <cctype>
#include <cstdlib>
#include <cmath>
#include <GL/glut.h>

void keyDown(unsigned char key, int, int) {
    lastInputTime = glutGet(GLUT_ELAPSED_TIME);

    // Call handleMenuNav and handleMenuSelect for key presses if in a menu
    if (gameState != 1) { // If in a menu/not in-game
        if (key == 13) {     // Carriage return (Enter)
            handleMenuSelect();
            return;
        }
        if (key == 'w' || key == 'W') { // Up navigation via W
            handleMenuNav(-1);
        }
        if (key == 's' || key == 'S') { // Down navigation via S
            handleMenuNav(1);
        }
    }

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
    
    // Dynamic Twin Tower Spawn
    if (key == '9') {
        spawnTwinTowers = true;
        twinTowerState = 1;
        twinTowerAnim = 0.0f;
        
        float speed2D = std::sqrt(vX*vX + vZ*vZ);
        if (speed2D > 10.0f) {
            twinTowerX = planeX + (vX / speed2D) * 4500.0f;
            twinTowerZ = planeZ + (vZ / speed2D) * 4500.0f; 
        } else {
            float yRad = yaw * 3.14159265f / 180.0f;
            twinTowerX = planeX - std::sin(yRad) * 4500.0f;
            twinTowerZ = planeZ - std::cos(yRad) * 4500.0f;
        }
    }

    if (lowerKey == 'g' && !crashed) {
        gearDeployed = !gearDeployed;
    }

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

    // L = Auto-Land (only if low & slow enough; glide down to ground then brake)

    if (lowerKey == 'l' && !crashed && !isGrounded) {
        float hGround = getVoxelHeight(planeX, planeZ);
        float agl = planeY - hGround;

        // Condition: Must be below 1000 height and under 400 speed
        if (agl < 1000.0f && currentSpeed < 400.0f) {
            autoLandOn    = true;
            autopilotOn   = false;
            gearDeployed  = true; // drop gear automatically
            afterburnerOn = false;
        } else {
            autoLandFailTimer = 3.0f; // display message for 3 seconds
            
            if (agl >= 1000.0f && currentSpeed >= 400.0f) {
                autoLandFailReason = "TOO HIGH & TOO FAST";
            } else if (agl >= 1000.0f) {
                autoLandFailReason = "ALTITUDE TOO HIGH";
            } else {
                autoLandFailReason = "SPEED TOO FAST";
            }
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
    
    // Quick menu navigation with up/down arrows
    if (gameState != 1) {
        if (key == GLUT_KEY_UP) {
            handleMenuNav(-1);
        } else if (key == GLUT_KEY_DOWN) {
            handleMenuNav(1);
        }
    }
}

void specialKeyUp(int key, int, int) {
    if (key >= 0 && key < 256) {
        specialKeys[key] = false;
    }
}

void updateControllerInput() {
#ifdef _WIN32
    static WORD lastButtons = 0;
    // Get state for player 1
    // We only poll controller 0.
    DWORD dwResult = XInputGetState(0, &controllerState);
    if (dwResult == ERROR_SUCCESS) {
        controllerConnected = true;

        WORD buttons = controllerState.Gamepad.wButtons;
        WORD pressed = (buttons ^ lastButtons) & buttons;

        if (pressed & XINPUT_GAMEPAD_START) {
            isPaused = !isPaused;
        }

        if (gameState != 1) { // In menus
            if (pressed & XINPUT_GAMEPAD_BACK) {
                gameState = 0;
            }
            if (pressed & XINPUT_GAMEPAD_DPAD_UP) {
                handleMenuNav(-1);
            }
            if (pressed & XINPUT_GAMEPAD_DPAD_DOWN) {
                handleMenuNav(1);
            }
            // A or Start to select
            if ((pressed & XINPUT_GAMEPAD_A) || (pressed & XINPUT_GAMEPAD_START)) {
                handleMenuSelect();
            }
            // B or Back to go back
            if ((pressed & XINPUT_GAMEPAD_B) || (pressed & XINPUT_GAMEPAD_BACK)) {
                // If we're not at the root menu, go back to it
                if (gameState != 0) {
                    gameState = 0;
                }
            }
        } else { // In game
            if (pressed & XINPUT_GAMEPAD_BACK) {
                keyDown('p', 0, 0); // Back/View button - Full Reset / Respawn
                keyUp('p', 0, 0);
            }
            if (pressed & XINPUT_GAMEPAD_RIGHT_THUMB) {
                keyDown('p', 0, 0); // R3 (Right Stick Click) - Full Reset / Respawn
                keyUp('p', 0, 0);
            }
            if (pressed & XINPUT_GAMEPAD_Y) {
                keyDown('9', 0, 0); // Y - Twin Towers
                keyUp('9', 0, 0);
            }
            if (pressed & XINPUT_GAMEPAD_X) {
                keyDown('l', 0, 0); // X - auto-land
                keyUp('l', 0, 0);
            }
            if (pressed & XINPUT_GAMEPAD_B) {
                gameState = 0;      // B - menu
            }
            if (pressed & XINPUT_GAMEPAD_A) {
                keyDown('v', 0, 0); // A - view (change camera)
                keyUp('v', 0, 0);
            }
            // Use DPAD for gear / altitude hold
            if (pressed & XINPUT_GAMEPAD_DPAD_UP) {
                keyDown('h', 0, 0);
                keyUp('h', 0, 0);
            }
            if (pressed & XINPUT_GAMEPAD_DPAD_DOWN) {
                keyDown('g', 0, 0); // Landing gear
                keyUp('g', 0, 0);
            }
            // Left Stick Click - Landing Gear
            if (pressed & XINPUT_GAMEPAD_LEFT_THUMB) {
                keyDown('g', 0, 0); // Landing gear
                keyUp('g', 0, 0);
            }
        }

        lastButtons = buttons;
    } else {
        controllerConnected = false;
    }
#endif
}

