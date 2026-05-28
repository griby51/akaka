#include "ExplosionManager.hpp"
#include <algorithm>

namespace explode{
    void ExplosionManager::spawn(float x, float y, const ExplosionConfig& cfg){
        mExplosions.emplace_back(x, y, cfg);
    }

    void ExplosionManager::update(float dt){
        for(auto& e : mExplosions){
            e.update(dt);
        }

        mExplosions.erase(
            std::remove_if(mExplosions.begin(), mExplosions.end(),
                [](const Explosion& e){
                    return e.isDone();
                }),
            mExplosions.end()
        );
    }

    void ExplosionManager::render(SDL_Renderer* renderer) const {
        for (auto& e : mExplosions){
            e.render(renderer);
        }
    }
}

