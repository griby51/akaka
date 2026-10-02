#include "Ability.hpp"
#include "LTimer.hpp"

float Ability::getCooldownProgress(){
    float progress = (float)timeSinceLast.getTicks() / (float)cooldown;
    if(progress > 1.f) progress = 1.f;
    return progress;
}
