#pragma once
#include "Missile.hpp"
#include "Projectile.hpp"
#include "Christmas.hpp"
#include <memory>
#include <vector>

namespace projectile{
    class ProjectileManager{
    public:
        void spawn(float x, float y, MissileConfig cfg);
        void spawn(float x, float y, GiftConfig cfg, int pastYExplosion);
        void spawn(float x, float y, ChristmasSleighConfig);
        void spawn(std::shared_ptr<Projectile> projectile);
        void update(float deltaTime);
        void render(SDL_Renderer* renderer);
    private:
        std::vector<std::shared_ptr<Projectile>> projectiles;
        std::vector<std::shared_ptr<Projectile>> pending;
    };
}
