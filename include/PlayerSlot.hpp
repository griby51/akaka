#pragma once
#include <string>

struct PlayerSlot{
    int presetIndex = -1;
    bool ready = false;
    
    std::string skinId = "";
    std::string hatId = "";
    std::string jetpackId = "";

    int skinIndex = 0;
    int hatIndex = 0;
    int jetpackIndex = 0;

    int joystickId = -1;
    int menuCursorY = 0;

    int lastAxisX = 0;
    int lastAxisY = 0;
};
