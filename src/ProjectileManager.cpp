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

    void ProjectileManager::spawn(std::shared_ptr<Projectile> projectile){
        if(projectile) projectile->setNetId(mNextNetId++);
        pending.push_back(std::move(projectile));
    }
}
