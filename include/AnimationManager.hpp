#pragma once

#include <unordered_map>
#include <string>

#include "Animation.hpp"

class AnimationManager{
public:
    static AnimationManager& getInstance(){
        static AnimationManager instance;
        return instance;
    }

    AnimationManager(const AnimationManager&) = delete;
    AnimationManager& operator=(const AnimationManager&) = delete;

    bool registerAnimation(const std::string& id, const Animation& animation);
    const Animation* getAnimation(const std::string& id);
    void clean();
private:
    AnimationManager() {}
    ~AnimationManager() { clean(); }
    std::unordered_map<std::string, Animation> animationMap;
};
