#include "ProjectileManager.hpp"
#include "Missile.hpp"
#include "Projectile.hpp"
#include <memory>

namespace projectile{
    void ProjectileManager::spawn(float x, float y, MissileConfig cfg){
        projectiles.push_back(std::make_unique<Missile>(x, y, cfg));
    }

    void ProjectileManager::update(float deltaTime){
        for (auto& p : projectiles){
            p->update(deltaTime);
        }

        projectiles.erase(
                std::remove_if(projectiles.begin(), projectiles.end(),
                    [](const std::unique_ptr<Projectile>& p){
                    return p->isDead();
                    }),
                projectiles.end()
                );
    }

    void ProjectileManager::render(SDL_Renderer* renderer){
        for(auto& p : projectiles){
            p->render(renderer);
        }
    }
}
