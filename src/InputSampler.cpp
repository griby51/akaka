#include "InputSampler.hpp"
#include "KeyPreset.hpp"
#include "PlayerInput.hpp"
#include <SDL2/SDL_joystick.h>
#include <SDL2/SDL_stdinc.h>

static constexpr int DEAD_ZONE = 8000;

namespace input{
    PlayerInput sample(const KeyPreset& preset, int joystickId, const Uint8* keys){
        PlayerInput in;

        if(joystickId != -1){
            SDL_Joystick* joystick = SDL_JoystickFromInstanceID(joystickId);
            if(!joystick) return in;
            Sint16 axisX = SDL_JoystickGetAxis(joystick, 0);

            in.left = axisX < -DEAD_ZONE;
            in.right = axisX > DEAD_ZONE;
            in.thrust = SDL_JoystickGetButton(joystick, 0);
            in.ability = SDL_JoystickGetButton(joystick, 1);

            return in;
        }

        if(!keys) return in;

        in.left = keys[preset.left];
        in.right = keys[preset.right];
        in.thrust = keys[preset.thrust];
        in.ability = keys[preset.missile];

        return in;
    }
}
