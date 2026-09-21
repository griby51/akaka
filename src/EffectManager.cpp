#include "EffectManager.hpp"
#include "Animation.hpp"
#include "AnimationManager.hpp"

#include <algorithm>
#include <cstdint>

void EffectManager::spawn(const std::string& id, float x, float y, float scale){
    const Animation* animation;
    animation = AnimationManager::getInstance().getAnimation(id);
    if(animation == nullptr) return;
    effects.push_back(Effect{AnimationPlayer(animation), x - (animation->frameW * scale ) / 2, y - (animation->frameH * scale) / 2, scale});
}

void EffectManager::update(float dt){
    for(auto& e : effects){
        e.player.update(dt);
    }

    effects.erase(
        std::remove_if(effects.begin(), effects.end(),
            [](const Effect& e){
            return e.player.isFinished();
            }),
        effects.end()
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

void EffectManager::render() const{
    for(const auto& e : effects){
        e.player.render(e.x, e.y, e.scale);
    }
}

void EffectManager::triggerShake(float intensity, float duration){
    if(intensity > shakeIntensity) shakeIntensity = intensity;
    if(duration > shakeDuration) shakeDuration = duration;
}
