#include "AnimationManager.hpp"
#include <cstdio>

bool AnimationManager::registerAnimation(const std::string& id, const Animation& animation){
    if(animation.frameCount <= 0 || animation.frameDuration <= 0 || animation.frameH <= 0 || animation.frameW <= 0 || animation.columns <= 0){
        printf("Animation : %s is invalid\n", id.c_str());
        return false;
    }

    if(animationMap.find(id) != animationMap.end()) return true;

    animationMap[id] = animation;
    return true;
}

const Animation* AnimationManager::getAnimation(const std::string& id){
    auto it = animationMap.find(id);
    if(it != animationMap.end()){
        return &it->second;
    }
    return nullptr;
}

void AnimationManager::clean(){
    animationMap.clear();
}
