#pragma once
#include "Particle.hpp"
#include "ExplosionManager.hpp"
#include "Explosion.hpp"
#include <SDL2/SDL.h>
#include "Config.hpp"
#include "LTimer.hpp"
#include <SDL2/SDL_mixer.h>
#include <math.h>
#include <vector>
#include "Player.hpp"

namespace missile{
    struct MissileConfig{
        std::vector<Player&> players;
        float precision = 3.0f;
        float velocity = 700.0f;
        float explosionTriggerRange = 50.0f;
        Mix_Chunk* launchSFX = NULL;
        LTexture* texture = NULL;
        SDL_Rect collider = {0, 0, 32, 32};
        bool showCollider = false;
        ParticleConfig particleConfig;
        explode::ExplosionManager& explosionManager;
        explode::ExplosionConfig explosionConfig;
        int particleNumber = 500;
        int explosionDelay = 500;
    };

    class Missile{
    public:
        Missile(float x, float y, MissileConfig& missileConfig);
        void update(float deltaTime);
        void render(SDL_Renderer* renderer);
        bool isAlive = false;
        SDL_Rect collider;
        void explode(explode::ExplosionManager& mgr, explode::ExplosionConfig& cfg);
    private:
        MissileConfig missileConfig;
        float angle;
        float x, y;
        float vx, vy;
        SDL_Rect* target;
        float currentVelocity = velocity;
        int particleSpawnTicks = 5;
        int currentParticle = 0;
        LTimer particleTimer;
        LTimer explosionTimer;
        bool explosionTriggered = false;
        std::vector<ThrustParticle> particles;
};
}

