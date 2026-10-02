#pragma once
#include "Projectile.hpp"
#include <memory>
#include <vector>

namespace projectile{
    class ProjectileManager{
    public:
        void spawn(std::shared_ptr<Projectile> projectile);
        void update(float deltaTime);
        const std::vector<std::shared_ptr<Projectile>>& all() const {return projectiles; }
    private:
        std::vector<std::shared_ptr<Projectile>> projectiles;
        std::vector<std::shared_ptr<Projectile>> pending;
        uint32_t mNextNetId = 1;
    };
}
