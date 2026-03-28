#pragma once

#include <SDL2/SDL.h>

struct Particle{
    float x, y;
    float vx, vy;
    float life;

    Particle(float px, float py);
    void update(float deltaTime);
    bool dead() const;
    void draw(SDL_Renderer* renderer);
}
