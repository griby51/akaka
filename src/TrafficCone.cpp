#include "TrafficCone.hpp"
#include "Explosion.hpp"
#include "ExplosionManager.hpp"
#include "Player.hpp"
#include "TextureManager.hpp"
#include "Utils.hpp"
#include <SDL2/SDL_render.h>

namespace projectile{
    TrafficCone::TrafficCone(float x, float y, TrafficConeConfig trafficConeConfig)
        : trafficConeConfig(trafficConeConfig){
            collider.x = x + trafficConeConfig.collider.x;
            collider.y = y + trafficConeConfig.collider.y;
            collider.w = trafficConeConfig.collider.w;
            collider.h = trafficConeConfig.collider.h;

            this->x = x;
            this->y = y;
            this->vx = 0.f;
            this->vy = 0.f;

            isAlive = true;

            particleTimer.start();
    }

    void TrafficCone::render(SDL_Renderer* renderer){
        if(!isAlive) return;

        TextureManager::getInstance().getTexture(trafficConeConfig.textureId)->render(x,y);

        if(trafficConeConfig.showCollider){
            SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
            SDL_RenderDrawRect(renderer, &collider);
        }
    }

    void TrafficCone::update(float deltaTime){
        if(!isAlive) return;

        std::vector<SDL_Rect*> playersCollider;

        for(int i = 0; i < trafficConeConfig.players->size(); i++){
            player::Player& player = (*trafficConeConfig.players)[i];
            if(!player.isAlive) continue;
            if(i == trafficConeConfig.throwerIndex) continue;

            if(collider.x - trafficConeConfig.triggerRange < player.collider.x && player.collider.x < collider.x + trafficConeConfig.triggerRange){
                if(!triggered){
                    triggered = true;
                    audioChannel = trafficConeConfig.audioManager->playSFX("missileLaunch");
                }
            }
            playersCollider.push_back(&player.collider);
        }
        
        if(triggered){
            y-=trafficConeConfig.speed * deltaTime;
            if(particleTimer.getTicks() >= particleSpawnTicks){
                int spawnX = collider.x + collider.w / 2;
                int spawnY = collider.y + collider.h;
                particleTimer.start();
                trafficConeConfig.particleManager->spawnThrustParticle(spawnX, spawnY, trafficConeConfig.particleConfig);
            }

            if(playersCollider.size() != 0){
            SDL_Rect* nearest = util::theNearest(collider, playersCollider);
                if(nearest->y > y){
                    explode(*trafficConeConfig.explosionManager, trafficConeConfig.explosionConfig);
                    isAlive = false;
                }
            }
        }else{
            x-=*trafficConeConfig.globalSpeed * deltaTime;
        }

        collider.x = x;
        collider.y = y;
    }

    void TrafficCone::explode(explode::ExplosionManager& mgr, explode::ExplosionConfig& cfg){
        mgr.spawn(x,y,cfg);

        float maxRadius = cfg.power * 50.f;
        float maxRadiusSq = maxRadius*maxRadius;
        float maxDmg = trafficConeConfig.maxDamage;
        float maxForce = cfg.power * 500.f;

        for(auto& player : *this->trafficConeConfig.players){
            if(!player.isAlive) continue;

            float distSq = util::distSq(this->collider, player.collider);
            if(distSq < maxRadiusSq){
                float factor = 1.f - (distSq / maxRadiusSq);
                player.updateLife(-maxDmg * factor);
                
                float dx = (player.collider.x + player.collider.w / 2.0f) - (this->collider.x + this->collider.w / 2.0f);
                float dy = (player.collider.y + player.collider.h / 2.0f) - (this->collider.y + this->collider.h / 2.0f);

                float forceX = (dx > 0.f ? maxForce : -maxForce) * factor;
                float forceY = (dy > 0.f ? maxForce : -maxForce) * factor;

                player.applyKnockBack(forceX, forceY);
            }
        }

        if(audioChannel) trafficConeConfig.audioManager->stopChannel(audioChannel);
        trafficConeConfig.audioManager->playSFX("explosion");

        mgr.triggerShake(8.f, 0.3f);
    }
}
