#include <windows.h>
#include <xinput.h>
#include <iostream>
int main() {
    XINPUT_STATE state;
    if (XInputGetState(0, &state) == ERROR_SUCCESS) {
        std::cout << "XInput works!\n";
    }
    return 0;
}
