#include "MissileManager.hpp"

namespace missile{
    void MissileManager::spawn(float x, float y, MissileConfig cfg){
        missiles.emplace_back(x, y, cfg);
    }

    void MissileManager::update(float deltaTime){
        for(auto& m : missiles){
            m.update(deltaTime);
        }

        missiles.erase(
            std::remove_if(missiles.begin(), missiles.end(),
                [](const Missile& m){
                    return !m.isAlive;
                }
            ),
            missiles.end()
        );
    }

    void MissileManager::render(SDL_Renderer* renderer){
        for(auto& m : missiles){
            m.render(renderer);
        }
    }
}
