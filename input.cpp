#include "input.h"
#include "globals.h"
#include <cctype>
#include <cstdlib>

void keyDown(unsigned char key, int, int) {

    keys[std::tolower(key)] = true;

    if (std::tolower(key) == 'v')
        cameraMode = (cameraMode + 1) % 4;

    if (key == '1') cameraMode = 0;
    if (key == '2') cameraMode = 1;
    if (key == '3') cameraMode = 2;
    if (key == '4') cameraMode = 3;

    if (key == 27) exit(0);
}

void keyUp(unsigned char key, int, int) {
    keys[std::tolower(key)] = false;
}