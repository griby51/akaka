#pragma once
#include <vector>
#include "Animation.hpp"

struct Effect{
    AnimationPlayer player;
    float x, y, scale;
};

class EffectManager{
public:
    void spawn(const std::string& id, float x, float y, float scale= 1.f);
    void update(float dt);
    void render() const;
private:
    std::vector<Effect> effects;
};
