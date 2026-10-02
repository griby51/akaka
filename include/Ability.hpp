#pragma once

#include "AudioManager.hpp"
#include "LTimer.hpp"

namespace player {class Player; }

class Ability{
public:
    virtual ~Ability() = default;
    virtual void use(player::Player* player) = 0;
    virtual void update(float deltaTime) {}
    float getCooldownProgress();
protected:
    int cooldown;
    int cost;
    LTimer timeSinceLast;
};
