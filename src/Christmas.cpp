#include "Christmas.hpp"
#include "Explosion.hpp"
#include "ExplosionManager.hpp"
#include "ProjectileManager.hpp"
#include "TextureManager.hpp"
#include "Utils.hpp"
#include "Player.hpp"

namespace projectile{
    Gift::Gift(float x, float y, GiftConfig giftConfig, int pastYExplosion)
        : giftConfig(giftConfig){
            this->x = x;
            this->y = y;
            this->vx = 0.f; 

            isAlive = true;
    }

    void Gift::render(SDL_Renderer* renderer){
        if(!isAlive) return;

        TextureManager::getInstance().getTexture(giftConfig.textureId)->render(x, y);
    }

    void Gift::update(float deltaTime){
        if(!isAlive) return;

        y+=giftConfig.speed * deltaTime;

        if (y > pastYExplosion){
            explode(*giftConfig.explosionManager, giftConfig.explosionConfig);
        }
    }

    void Gift::explode(explode::ExplosionManager& mgr, explode::ExplosionConfig& cfg){
        mgr.spawn(x, y, cfg);
        float maxRadius = cfg.power * 50.f;
        float maxRadiusSq = maxRadius*maxRadius;
        float maxDmg = giftConfig.maxDamage;
        float maxForce = cfg.power * 500.f;

        for(auto& player : *this->giftConfig.players){
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

        mgr.triggerShake(5, 0.3);
    }

    ChristmasSleigh::ChristmasSleigh(float x, float y, ChristmasSleighConfig christmasSleighConfig)
        : christmasSleighConfig(christmasSleighConfig){
        
        y = 60;
        x = -60;
        timeUntilNextGift = rand() % 5000;
        dropGiftTimer.start();
    }

    void ChristmasSleigh::update(float deltaTime){
        x+=christmasSleighConfig.speed*deltaTime;
        player::Player& rider = (*christmasSleighConfig.players)[christmasSleighConfig.throwerIndex];
        rider.teleportTo(x + 20, y);
        if(dropGiftTimer.getTicks() >= timeUntilNextGift){
            timeUntilNextGift = rand() % 5000;
            christmasSleighConfig.projectileManager->spawn(x, y, christmasSleighConfig.giftConfig, rand() % (christmasSleighConfig.screenHeight - 100) + 50);
        }
    }
}
