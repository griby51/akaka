#pragma once

#include <SDL2/SDL.h>
#include "Particle.hpp"

class ParticleManager{
public:
    ParticleManager();

    void update(float deltaTime);
    void render(SDL_Renderer* renderer);

    void spawnThrustParticle(int x, int y, const ParticleConfig& config);

private:
    static constexpr int MAX_THRUST_PARTICLES = 2000;

    ThrustParticle thrustPool[MAX_THRUST_PARTICLES];
    int currentThrustIdx;
};
