#pragma once

#include "AudioManager.hpp"
#include "Christmas.hpp"
#include "ProjectileManager.hpp"
#include "Missile.hpp"
#include "LTimer.hpp"
#include "TrafficCone.hpp"

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

class MissileAbility : public Ability{
public:
    MissileAbility(projectile::ProjectileManager* projectileManager, projectile::MissileConfig missileConfig, int screenWidth, int screenHeight);
    void use(player::Player* player) override;
private:
    projectile::ProjectileManager* projectileManager;
    projectile::MissileConfig missileConfig;
    int screenWidth, screenHeight;
};

class TrafficConeAbility : public Ability{
public:
    TrafficConeAbility(projectile::ProjectileManager* projectileManager, projectile::TrafficConeConfig trafficConeConfig, int screenWidth, int screenHeight);
    void use(player::Player* player) override;
private:
    projectile::ProjectileManager* projectileManager;
    projectile::TrafficConeConfig trafficConeConfig;
    int screenWidth, screenHeight;
};

class ChristmasSleighAbility : public Ability{
public:
    ChristmasSleighAbility(projectile::ProjectileManager* projectileManager, projectile::ChristmasSleighConfig christmasSleighConfig);
    void use(player::Player* player) override;
private:
    projectile::ProjectileManager* projectileManager;
    projectile::ChristmasSleighConfig christmasSleighConfig;
};
