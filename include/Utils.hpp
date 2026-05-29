#pragma once
#include <SDL2/SDL.h>

namespace util{
    bool collide(SDL_Rect& a, SDL_Rect& b);
    bool isWithinDistance(SDL_Rect& a, SDL_Rect& b, float threshold);
}
