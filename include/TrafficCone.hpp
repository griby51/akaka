#pragma once
#include "Explosion.hpp"
#include "Particle.hpp"
#include "ExplosionManager.hpp"
#include "AudioManager.hpp"
#include "LTimer.hpp"
#include "Projectile.hpp"

#include <SDL2/SDL.h>
#include <vector>

namespace player{class Player;}

namespace projectile{
    struct TrafficConeConfig{
        std::vector<player::Player>* players = nullptr;

        int throwerIndex = -1;
        float speed = 2000.f;
        float explosionTriggerRange = 30.f;
        float* globalSpeed = nullptr;
        float triggerRange;
        std::string textureId = "trafficCone";
        SDL_Rect collider = {0, 0, 32, 32};
        bool showCollider = false;
        ParticleConfig particleConfig;
        explode::ExplosionManager* explosionManager;
        explode::ExplosionConfig explosionConfig;
        float maxDamage = 50.f;
        int particleNumber = 500;
        AudioManager* audioManager;
    };

    class TrafficCone : public Projectile{
    public:
        TrafficCone(float x, float y, TrafficConeConfig trafficConeConfig);
        void update(float deltaTime) override;
        void render(SDL_Renderer* renderer) override;
        void explode(explode::ExplosionManager& mgr, explode::ExplosionConfig& cfg);
    private:
        TrafficConeConfig trafficConeConfig;
        
        int particleSpawnTicks = 5;
        int currentParticle = 0;

        LTimer particleTimer;
        LTimer explosionTimer;
        bool triggered = false;
        std::vector<ThrustParticle> particles;

        int audioChannel;
    };

};
