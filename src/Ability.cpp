#include "Ability.hpp"
#include "Config.hpp"
#include "Explosion.hpp"
#include "ExplosionManager.hpp"
#include "Missile.hpp"
#include "Player.hpp"
#include "ProjectileManager.hpp"
#include "Utils.hpp"
#include <SDL2/SDL_render.h>

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
        for(int i = 0; i < missileConfig.players->size(); i++){
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

FreezeAbility::FreezeAbility(int duration, int radius, std::vector<player::Player>* players) : duration(duration), radius(radius), players(players) {
    cooldown = 1000;
    cost = 500;
    timeSinceLast.start();
}

void FreezeAbility::use(player::Player* player){
    if(timeSinceLast.getTicks() <= cooldown) return;
    if(player->getScore() < cost) return;
    int playerTouched = 0;
    for(int i = 0; i < players->size(); i++){
        if(&(*players)[i] != player){
            if(util::isWithinDistance(player->collider, (*players)[i].collider, (float)radius)){
                (*players)[i].setVelocity(0, 0);
                playerTouched++;
            }
        }
    }

    if(playerTouched == 0) return;
    player->updateScore(-cost);
    timeSinceLast.start(); 
}

KamikazeAbility::KamikazeAbility(std::vector<player::Player>* players, explode::ExplosionManager& mgr, explode::ExplosionConfig cfg, AudioManager* audioManager, float maxDamage, float maxRadius, float maxForce) : players(players), cfg(cfg), mgr(mgr), audioManager(audioManager),maxDamage(maxDamage){

    timeSinceLast.start();

    if(maxRadius < 0){
        this->maxRadius = cfg.power * 50;
    }else this->maxRadius = maxRadius;

    if(maxForce < 0){
        this->maxForce = cfg.power * 500.f;
    }else this->maxForce = maxForce;

    printf("Power, maxRadius : %f %f", cfg.power, maxRadius);

    cost = 100;
    cooldown = 2000;
}

void KamikazeAbility::use(player::Player* player){
    if(timeSinceLast.getTicks() <= cooldown) return;
    if(player->getScore() < cost) return;

    int centerX = player->collider.x + (player->collider.w / 2);
    int centerY = player->collider.y + (player->collider.h / 2);
    mgr.spawn(centerX, centerY, cfg);

    audioManager->playSFX("explosion");

    mgr.triggerShake(16.f, 0.3f);

    float maxRadiusSq = maxRadius*maxRadius;

    for (auto& p : *players){
        if(&p == player) continue;

        float distSq = util::distSq(player->collider, p.collider);

        if(distSq < maxRadiusSq){
            float factor = 1.f - (distSq / maxRadiusSq);

            p.updateLife(-maxDamage * factor);

            float dx = (p.collider.x + p.collider.w / 2.f) - (player->collider.x + player->collider.w / 2.f);
            float dy = (p.collider.y + p.collider.h / 2.f) - (player->collider.y + player->collider.h / 2.f);

            float forceX = (dx > 0.f ? maxForce : -maxForce) * factor;
            float forceY = (dy > 0.f ? maxForce : -maxForce) * factor;

            p.applyKnockBack(forceX, forceY);
        }
    }

    timeSinceLast.start();

    player->updateScore(-cost);
}

