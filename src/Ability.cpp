#include "Ability.hpp"
#include "Christmas.hpp"
#include "Explosion.hpp"
#include "ExplosionManager.hpp"
#include "Missile.hpp"
#include "Player.hpp"
#include "ProjectileManager.hpp"
#include "TrafficCone.hpp"
#include "Utils.hpp"
#include <SDL2/SDL_render.h>

float Ability::getCooldownProgress(){
    float progress = (float)timeSinceLast.getTicks() / (float)cooldown;
    if(progress > 1.f) progress = 1.f;
    return progress;
}

MissileAbility::MissileAbility(projectile::ProjectileManager* projectileManager, projectile::MissileConfig missileConfig, int screenWidth, int screenHeight) :
    projectileManager(projectileManager), missileConfig(missileConfig), screenWidth(screenWidth), screenHeight(screenHeight){
    cooldown = 5000;
    cost = 200;
    timeSinceLast.start();
}

void MissileAbility::use(player::Player* player){
    if(timeSinceLast.getTicks() <= cooldown) return;
    if(player->getScore() < cost) return;
    int myIndex = -1;
    if(missileConfig.players){
        for(size_t i = 0; i < missileConfig.players->size(); i++){
            if(&(*missileConfig.players)[i] == player){
                myIndex = i;
                break;
            }
        }
    }

    missileConfig.throwerIndex = myIndex;

    SDL_Point p = util::spawnOffScreen(screenWidth, screenHeight, 200);

    projectileManager->spawn(p.x, p.y, missileConfig);
    player->updateScore(-cost);
    timeSinceLast.start();
}

TrafficConeAbility::TrafficConeAbility(projectile::ProjectileManager* projectileManager, projectile::TrafficConeConfig trafficConeConfig, int screenWidth, int screenHeight) :
    projectileManager(projectileManager), trafficConeConfig(trafficConeConfig), screenWidth(screenWidth), screenHeight(screenHeight){
        cooldown = 2000;
        cost = 200;
        timeSinceLast.start();
}

void TrafficConeAbility::use(player::Player* player){
    if(timeSinceLast.getTicks() <= cooldown) return;
    if(player->getScore() < cost) return;
    int myIndex = -1;
    if(trafficConeConfig.players){
        for(size_t i = 0; i < trafficConeConfig.players->size(); i++){
            if(&(*trafficConeConfig.players)[i] == player){
                myIndex = i;
                break;
            }
        }
    }

    trafficConeConfig.throwerIndex = myIndex;
    projectileManager->spawn(screenWidth + 10, screenHeight - trafficConeConfig.collider.h, trafficConeConfig);
    player->updateScore(-cost);
    timeSinceLast.start();
}


ChristmasSleighAbility::ChristmasSleighAbility(projectile::ProjectileManager* projectileManager, projectile::ChristmasSleighConfig christmasSleighConfig)
    : projectileManager(projectileManager), christmasSleighConfig(christmasSleighConfig){
        cooldown = 20000;
        cost = 1000;
        timeSinceLast.start();
}

void ChristmasSleighAbility::use(player::Player* player){
    if(timeSinceLast.getTicks() <= cooldown) return;
    if(player->getScore() < cost) return;

    int myIndex = -1;
    if(christmasSleighConfig.players){
        for(size_t i = 0; i < christmasSleighConfig.players->size(); i++){
            if(&(*christmasSleighConfig.players)[i] == player){
                myIndex = i;
                break;
            }
        }
    }

    christmasSleighConfig.throwerIndex = myIndex;

    projectileManager->spawn(0, 0, christmasSleighConfig);

    player->updateScore(-cost);
    timeSinceLast.start();
}
