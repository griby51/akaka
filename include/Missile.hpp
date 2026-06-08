#pragma once
#include "Particle.hpp"
#include "ExplosionManager.hpp"
#include "AudioManager.hpp"
#include "Explosion.hpp"
#include "LTimer.hpp"
#include "Projectile.hpp"

#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include <math.h>
#include <vector>

namespace player {class Player; }

namespace projectile{
    struct MissileConfig{
        std::vector<player::Player>* players = nullptr;

        int throwerIndex = -1;
        float precision = 3.f;
        float velocity = 1000.0f;
        float explosionTriggerRange = 70.0f;
        Mix_Chunk* launchSFX = NULL;
        std::string textureId = "missile";
        SDL_Rect collider = {0, 0, 32, 32};
        bool showCollider = false;
        ParticleConfig particleConfig;
        explode::ExplosionManager* explosionManager;
        explode::ExplosionConfig explosionConfig;
        float maxDamage = 50.f;
        int particleNumber = 500;
        int explosionDelay = 70;
        AudioManager* audioManager;
    };

    class Missile : public Projectile{
    public:
        Missile(float x, float y, MissileConfig missileConfig);
        void update(float deltaTime) override;
        void render(SDL_Renderer* renderer) override;
        void explode(explode::ExplosionManager& mgr, explode::ExplosionConfig& cfg);
    private:
        MissileConfig missileConfig;
        float angle;
        SDL_Rect* target;
        float currentVelocity = 0.f;
        int particleSpawnTicks = 5;
        int currentParticle = 0;
        LTimer particleTimer;
        LTimer explosionTimer;
        bool explosionTriggered = false;
        std::vector<ThrustParticle> particles;

        int audioChannel;
    };
}

