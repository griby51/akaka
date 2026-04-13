#pragma once
#include "LTexture.hpp"

struct PlayerSlot{
    int presetIndex = -1;
    bool ready = false;
    LTexture* skin;
    LTexture* hat;
};
