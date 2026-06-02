#include "ExplosionManager.hpp"
#include <algorithm>
#include <cstdlib>

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

        if(shakeDuration > 0.f){
            shakeDuration -= dt;

            shakeIntensity -= 20.f * dt;
            if(shakeIntensity < 0.f) shakeIntensity = 0.f;

            if(shakeIntensity >= 1.f){
                int range = (int)shakeIntensity * 2 + 1;
                shakeX = (rand() % range) - (int)shakeIntensity;
                shakeY = (rand() % range) - (int)shakeIntensity;
            }else{
                shakeX = 0;
                shakeY = 0;
            }
        }else{
            shakeX = 0;
            shakeY = 0;
            shakeIntensity = 0.f;
        }
    }

    void ExplosionManager::render(SDL_Renderer* renderer) const {
        for (auto& e : mExplosions){
            e.render(renderer);
        }
    }
    
    void ExplosionManager::triggerShake(float intensity, float duration){
        if(intensity > shakeIntensity) shakeIntensity = intensity;
        if(duration > shakeDuration) shakeDuration = duration;
    }
}

