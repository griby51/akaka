#include "ProjectileManager.hpp"
#include "Projectile.hpp"
#include <algorithm>
#include <memory>

namespace projectile{
    void ProjectileManager::update(float deltaTime){
        for (auto& p : projectiles){
            p->update(deltaTime);
        }

        projectiles.erase(
                std::remove_if(projectiles.begin(), projectiles.end(),
                    [](const std::shared_ptr<Projectile>& p){
                    return p->isDead();
                    }),
                projectiles.end()
                );

        for(auto& p : pending){
            projectiles.push_back(std::move(p));
        }
        pending.clear();
    }

    void ProjectileManager::render(SDL_Renderer* renderer){
        for(auto& p : projectiles){
            p->render(renderer);
        }
    }

    void ProjectileManager::spawn(std::shared_ptr<Projectile> projectile){
        pending.push_back(std::move(projectile));
    }
}
