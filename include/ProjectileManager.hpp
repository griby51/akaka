#pragma once
#include "Projectile.hpp"
#include <memory>
#include <vector>

namespace projectile{
    class ProjectileManager{
    public:
        void spawn(std::shared_ptr<Projectile> projectile);
        void update(float deltaTime);
        void render(SDL_Renderer* renderer);
    private:
        std::vector<std::shared_ptr<Projectile>> projectiles;
        std::vector<std::shared_ptr<Projectile>> pending;
    };
}
