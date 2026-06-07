#pragma once
#include <SDL2/SDL.h>
#include <vector>

namespace util{
    bool collide(SDL_Rect& a, SDL_Rect& b);
    bool isWithinDistance(SDL_Rect& a, SDL_Rect& b, float threshold);
    SDL_Rect* theNearest(SDL_Rect& tested, std::vector<SDL_Rect*> targets);
    SDL_Point spawnOffScreen(int screenWidth, int screenHeight, int margin);
    float distSq(SDL_Rect& a, SDL_Rect& b);

    void drawRoundedRect(SDL_Renderer* renderer, SDL_Rect rect, int radius, SDL_Color color);
}
