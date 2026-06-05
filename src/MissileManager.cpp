#include "MissileManager.hpp"

namespace missile{
    void MissileManager::spawn(float x, float y, MissileConfig cfg){
        missiles.push_back(std::make_unique<Missile>(x, y, cfg));
    }

    void MissileManager::update(float deltaTime){
        for(auto& m : missiles){
            m->update(deltaTime);
        }

        missiles.erase(
            std::remove_if(missiles.begin(), missiles.end(),
                [](const std::unique_ptr<Missile>& m){
                    return !m->isAlive;
                }
            ),
            missiles.end()
        );
    }

    void MissileManager::render(SDL_Renderer* renderer){
        for(auto& m : missiles){
            m->render(renderer);
        }
    }
}
