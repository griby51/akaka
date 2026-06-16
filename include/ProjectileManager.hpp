#pragma once
#include "Missile.hpp"
#include "Projectile.hpp"
#include "TrafficCone.hpp"
#include <memory>
#include <vector>

namespace projectile{
    class ProjectileManager{
    public:
        void spawn(float x, float y, MissileConfig cfg);
        void spawn(float x, float y, TrafficConeConfig cfg);
        void update(float deltaTime);
        void render(SDL_Renderer* renderer);
    private:
        std::vector<std::unique_ptr<Projectile>> projectiles;
    };
}
