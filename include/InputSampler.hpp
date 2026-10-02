#pragma once

#include <SDL2/SDL.h>
#include "KeyPreset.hpp"
#include "PlayerInput.hpp"

namespace input{
    PlayerInput sample(const KeyPreset& preset, int joystickId, const Uint8* keys);
}
