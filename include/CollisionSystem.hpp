#pragma once
#include <SDL2/SDL.h>

namespace Collision{
    bool collide(SDL_Rect* collider1, SDL_Rect* collider2);
}