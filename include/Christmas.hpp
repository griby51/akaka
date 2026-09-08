#pragma once
#include "Explosion.hpp"
#include "ExplosionManager.hpp"
#include "AudioManager.hpp"
#include "LTimer.hpp"
#include "Projectile.hpp"

#include <SDL2/SDL.h>
#include <vector>

namespace player{class Player;}
namespace projectile{class ProjectileManager}

namespace projectile{

    struct GiftConfig{
        std::vector<player::Player>* players = nullptr;
        float speed = 2000.f;
        std::string textureId = "gift";
        explode::ExplosionManager* explosionManager;
        explode::ExplosionConfig explosionConfig;
        float maxDamage = 50.f;
        AudioManager* audioManager;
    };

    struct ChristmasSleighConfig{
        std::vector<player::Player>* players = nullptr;

        int throwerIndex = -1;
        float speed = 2000.f;
        
        std::string textureId = "christmasSleigh";
        projectile::ProjectileManager* projectileManager;
        
        GiftConfig giftConfig;
        
        AudioManager* audioManager;
        int screenHeight = 600;
        int screenWidth = 800;
    };

    class ChristmasSleigh : public Projectile{
    public:
        ChristmasSleigh(float x, float y, ChristmasSleighConfig christmasSleighConfig);
        void update(float deltaTime) override;
        void render(SDL_Renderer* renderer) override;
    private:
        ChristmasSleighConfig christmasSleighConfig;

        LTimer dropGiftTimer;
        int timeUntilNextGift = 0;
    };

    class Gift : public Projectile{
    public:
        Gift(float x, float y, GiftConfig giftConfig, int pastYExplosion);
        void update(float deltaTime) override;
        void render(SDL_Renderer* renderer) override;
        void explode(explode::ExplosionManager& mgr, explode::ExplosionConfig& cfg);
    private:
        GiftConfig giftConfig;

        int pastYExplosion;
    };
};
