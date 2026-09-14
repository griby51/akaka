#include "EffectManager.hpp"
#include "Animation.hpp"
#include "AnimationManager.hpp"

#include <algorithm>

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
}

void EffectManager::render() const{
    for(const auto& e : effects){
        e.player.render(e.x, e.y, e.scale);
    }
}
