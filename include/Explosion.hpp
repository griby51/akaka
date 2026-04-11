#pragma once
#include <SDL2/SDL.h>
#include "Particle.hpp"

class Explosion{
public:
    void trigger(int x, int y);
    void update(float deltaTime);
    void render(SDL_Renderer* renderer);
    bool isAlive();
protected:
    ExplosionParticle debris[30];
    EmberParticle embers[50];
    SmokeParticle smoke[10];
}
