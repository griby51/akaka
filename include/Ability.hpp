#pragma once

#include "AudioManager.hpp"
#include "Explosion.hpp"
#include "ExplosionManager.hpp"
#include "ProjectileManager.hpp"
#include "Missile.hpp"
#include "LTimer.hpp"

namespace player {class Player; }

class Ability{
public:
    virtual ~Ability() = default;
    virtual void use(player::Player* player) = 0;
    virtual void update(float deltaTime) {}
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

class FreezeAbility : public Ability{
public:
    FreezeAbility(int duration, int radius, std::vector<player::Player>* players);
    void use(player::Player* player) override;
private:
    int duration, radius;
    std::vector<player::Player>* players;
};

class KamikazeAbility : public Ability{
public:
    KamikazeAbility(std::vector<player::Player>* players, explode::ExplosionManager& mgr, explode::ExplosionConfig cfg, AudioManager* audioManager,float maxDamage = 50.f, float maxRadius = -1.f, float maxForce = -1);
    void use(player::Player* player) override;
private:
    std::vector<player::Player>* players;
    float maxDamage, maxRadius, maxForce;
    explode::ExplosionConfig cfg;
    explode::ExplosionManager& mgr;
    AudioManager* audioManager;
    
};
