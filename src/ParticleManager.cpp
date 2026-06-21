#include "ParticleManager.hpp"
#include "Particle.hpp"

ParticleManager::ParticleManager(){
    currentThrustIdx = 0;
}

void ParticleManager::update(float deltaTime){
    for(int i = 0; i < MAX_THRUST_PARTICLES; i++){
        if(thrustPool[i].isAlive){
            thrustPool[i].update(deltaTime);
        }
    }
}

void ParticleManager::render(SDL_Renderer* renderer){
    for(int i = 0; i < MAX_THRUST_PARTICLES; i++){
        int orderedIndex = (currentThrustIdx - 1 - i + MAX_THRUST_PARTICLES) % MAX_THRUST_PARTICLES;
        if(thrustPool[orderedIndex].isAlive){
            thrustPool[orderedIndex].render(renderer);
        }
    }
}

void ParticleManager::spawnThrustParticle(int x, int y, const ParticleConfig& config){
    ThrustParticle& p = thrustPool[currentThrustIdx];

    p.init(config);
    p.setPos(x, y);

    currentThrustIdx = (currentThrustIdx + 1) % MAX_THRUST_PARTICLES;
}
