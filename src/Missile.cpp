#include "Missile.hpp"
#include "AudioManager.hpp"
#include "Player.hpp"
#include "Explosion.hpp"
#include "ExplosionManager.hpp"
#include "Utils.hpp"
#include "TextureManager.hpp"

#include <SDL2/SDL_mixer.h>
#include <math.h>

namespace projectile{
    Missile::Missile(float x, float y, MissileConfig missileConfig)
    : missileConfig(missileConfig){
        collider.x = x + missileConfig.collider.x;
        collider.y = y + missileConfig.collider.y;
        collider.w = missileConfig.collider.w;
        collider.h = missileConfig.collider.h;

        this->x = x;
        this->y = y;
        this->vx=0.f;
        this->vy=0.f;

        angle = 0.f;

        isAlive = true;

        particles.resize(missileConfig.particleNumber);
        for(int i = 0; i < missileConfig.particleNumber; i++){
            particles[i].init(&this->missileConfig.particleConfig);
            particles[i].reset();
            particles[i].setPos(10000, 10000);
        }
        particleTimer.start();
        audioChannel = missileConfig.audioManager->playSFX("missileLaunch");
    }

    void Missile::render(SDL_Renderer* renderer){
        if (!isAlive) return;
        for(int i = 0; i < missileConfig.particleNumber; i++){
            particles[i].render(renderer);
        }
        TextureManager::getInstance().getTexture(missileConfig.textureId)->render(x, y, NULL, angle * (180 / M_PI) + 90);

        if(missileConfig.showCollider){
            SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
            SDL_RenderDrawRect(renderer, &collider);
        }
    }

    void Missile::update(float deltaTime){
        if (!isAlive) return;
        for(int i = 0; i < missileConfig.particleNumber; i++){
            particles[i].update(deltaTime);
        }

        std::vector<SDL_Rect*> playersColliders;
        for(int i = 0; i < missileConfig.players->size(); i++){
            player::Player& player = (*missileConfig.players)[i];
            if(!player.isAlive) continue;
            if(i == missileConfig.throwerIndex) continue;
            playersColliders.push_back(&player.collider);
        }

        SDL_Rect* target;

        if(playersColliders.size() == 0){
            target = &(*missileConfig.players)[missileConfig.throwerIndex].collider; 
        }else{
            target = util::theNearest(collider, playersColliders);
        }


        int targetX = target->x + (target->w / 2);
        int targetY = target->y + (target->h / 2);
        float xDist = targetX - x;
        float yDist = targetY - y;
        float dist = sqrt(pow(xDist, 2) + pow(yDist, 2));

        double targetedAngle = atan2(yDist, xDist);

        double diff = targetedAngle - angle;
        while (diff > M_PI) diff -= 2 * M_PI;
        while (diff < -M_PI) diff += 2 * M_PI;

        float maxTurn = missileConfig.precision * deltaTime;

        if(diff > maxTurn){
            angle += maxTurn;
        }else if(diff < -maxTurn){
            angle -= maxTurn;
        }else{
            angle= targetedAngle;
        }
        float alignRatio = 1.0f - (std::abs(diff) / M_PI);
        currentVelocity = missileConfig.velocity * std::max(0.8f, alignRatio);

        vx = std::cos(angle) * currentVelocity;
        vy = std::sin(angle) * currentVelocity;

        x+=vx * deltaTime;
        y+=vy * deltaTime;
        collider.x = x;
        collider.y = y;

        float centerX = x + 16;
        float centerY = y + 16;

        float backX = centerX - std::cos(angle) * 16;
        float backY = centerY - std::sin(angle) * 16;

        if(particleTimer.getTicks() >= particleSpawnTicks){
            particleTimer.start();
            particles[currentParticle].setPos(backX, backY);
            particles[currentParticle].reset();
            currentParticle = (currentParticle + 1) % missileConfig.particleNumber;
        }

        if(dist < missileConfig.explosionTriggerRange){
            if(!explosionTriggered){
                explosionTimer.start();
                explosionTriggered = true;
            }
        }

        if(explosionTriggered && explosionTimer.getTicks() >= missileConfig.explosionDelay){
            explode(*missileConfig.explosionManager, missileConfig.explosionConfig);
            isAlive = false;
        }
    }

    void Missile::explode(explode::ExplosionManager& mgr, explode::ExplosionConfig& cfg){
        mgr.spawn(x, y, cfg);
        
        float maxRadius = cfg.power * 50.f;
        float maxRadiusSq = maxRadius * maxRadius;
        float maxDamage = missileConfig.maxDamage;
        float maxForce = cfg.power * 500.f;

        for(auto& player : *this->missileConfig.players){
            if(!player.isAlive) continue;

            float distSq = util::distSq(this->collider, player.collider);

            if(distSq < maxRadiusSq){
                float factor = 1.f - (distSq / maxRadiusSq);

                player.updateLife(-maxDamage * factor);

                float dx = (player.collider.x + player.collider.w / 2.0f) - (this->collider.x + this->collider.w / 2.0f);
                float dy = (player.collider.y + player.collider.h / 2.0f) - (this->collider.y + this->collider.h / 2.0f);

                float forceX = (dx > 0.f ? maxForce : -maxForce) * factor;
                float forceY = (dy > 0.f ? maxForce : -maxForce) * factor;

                player.applyKnockBack(forceX, forceY);

            }
        }

        missileConfig.audioManager->stopChannel(audioChannel);
        missileConfig.audioManager->playSFX("explosion");

        mgr.triggerShake(8.f, 0.3f);

        isAlive = false;
    }
}
