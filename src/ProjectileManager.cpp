#include "ProjectileManager.hpp"
#include "Christmas.hpp"
#include "Missile.hpp"
#include "Projectile.hpp"
#include "TrafficCone.hpp"
#include <memory>

namespace projectile{
    void ProjectileManager::spawn(float x, float y, MissileConfig cfg){
        pending.push_back(std::make_unique<Missile>(x, y, cfg));
    }

    void ProjectileManager::spawn(float x, float y, TrafficConeConfig cfg){
        pending.push_back(std::make_unique<TrafficCone>(x, y, cfg));
    }

    void ProjectileManager::spawn(float x, float y, ChristmasSleighConfig cfg){
        pending.push_back(std::make_unique<ChristmasSleigh>(x, y, cfg));
    }

    void ProjectileManager::spawn(float x, float y, GiftConfig cfg, int yMax){
        pending.push_back(std::make_unique<Gift>(x, y, cfg, yMax));
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
}
