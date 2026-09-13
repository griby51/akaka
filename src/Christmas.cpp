#include "Christmas.hpp"
#include "Explosion.hpp"
#include "ExplosionManager.hpp"
#include "ProjectileManager.hpp"
#include "TextureManager.hpp"
#include "Utils.hpp"
#include "Player.hpp"

namespace projectile{
    Gift::Gift(float x, float y, GiftConfig giftConfig, int pastYExplosion)
        : giftConfig(giftConfig), pastYExplosion(pastYExplosion){
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
            isAlive = false;
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
            SDL_Rect giftPos = {(int)x, int(y), 0, 0};

            float distSq = util::distSq(giftPos, player.collider);
            if(distSq < maxRadiusSq){
                float factor = 1.f - (distSq / maxRadiusSq);
                player.updateLife(-maxDmg * factor);
                
                float dx = (player.collider.x + player.collider.w / 2.0f) - (giftPos.x + giftPos.w / 2.0f);
                float dy = (player.collider.y + player.collider.h / 2.0f) - (giftPos.y + giftPos.h / 2.0f);

                float forceX = (dx > 0.f ? maxForce : -maxForce) * factor;
                float forceY = (dy > 0.f ? maxForce : -maxForce) * factor;

                player.applyKnockBack(forceX, forceY);
            }
        }

        giftConfig.audioManager->playSFX("explosion");

        mgr.triggerShake(5, 0.3);
    }

    ChristmasSleigh::ChristmasSleigh(float x, float y, ChristmasSleighConfig christmasSleighConfig)
        : christmasSleighConfig(christmasSleighConfig){
        
        y = 60;
        x = -60;
        timeUntilNextGift = rand() % 5000;
        dropGiftTimer.start();
        isAlive = true;
    }

    void ChristmasSleigh::update(float deltaTime){
        player::Player& rider = (*christmasSleighConfig.players)[christmasSleighConfig.throwerIndex];
        if(x > christmasSleighConfig.screenWidth + 50){
            if(rider.isControlled){
                rider.isControlled = false;
                rider.teleportTo(christmasSleighConfig.screenWidth / 2, christmasSleighConfig.screenHeight / 2);
            }
            isAlive = false;
            return;
        }
        x+=christmasSleighConfig.speed*deltaTime;
        rider.teleportTo(x + 20, y);
        if(!rider.isControlled){
            rider.isControlled = true;
        }
        if(dropGiftTimer.getTicks() >= timeUntilNextGift){
            timeUntilNextGift = rand() % 1500 + 500;
            christmasSleighConfig.projectileManager->spawn(x, y, christmasSleighConfig.giftConfig, rand() % (christmasSleighConfig.screenHeight - 150) + 100);
            dropGiftTimer.start();
        }
    }

    void ChristmasSleigh::render(SDL_Renderer* renderer){
        if(!isAlive) return;
        
        TextureManager::getInstance().getTexture(christmasSleighConfig.textureId)->render(x, y);
    }
}
