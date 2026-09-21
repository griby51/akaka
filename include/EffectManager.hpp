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
    void triggerShake(float intensity, float duration);
    int getShakeX() const {return shakeX;}
    int getShakeY() const {return shakeY;}

    void update(float dt);
    void render() const;
private:
    float shakeIntensity = 0.f;
    float shakeDuration = 0.f;
    int shakeX = 0;
    int shakeY = 0;
    std::vector<Effect> effects;
};
