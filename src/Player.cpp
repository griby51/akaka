#include "Player.hpp"
#include "KeyPreset.hpp"
#include "Utils.hpp"
#include <SDL2/SDL_mixer.h>

namespace player{
    Player::Player(PlayerConfig&& config) : config(std::move(config)){
        isAlive = true;
        thrustParticlesTimer.start();

        x = config.screenWidth / 2.0f;
        y = config.screenHeight / 4.0f;

        vx = 0.f;
        vy =0.f;
        dir = 0;

        collider.x = config.collider.x + x;
        collider.y = config.collider.y + y;
        collider.w = config.collider.w;
        collider.h = config.collider.h;

        life = config.maxHealth;
        score = 0;
    }

    void Player::updateScore(int toAdd){
        score += toAdd;
    }

    void Player::render(SDL_Renderer* renderer){
        if (!isAlive) return;

        config.skin->render(x, y);
        if(config.hat != nullptr){
            config.hat->render(x, y);
        }

        if(config.showCollider){
            SDL_SetRenderDrawColor(renderer, config.colliderColor.r, config.colliderColor.g, config.colliderColor.b, config.colliderColor.a);
            SDL_RenderDrawRect(renderer, &config.collider);
        }
    }

    void Player::update(float deltaTime){
        if(!isAlive) return;

        if(config.ability){
            config.ability->update(deltaTime);
        }

        if(isControlled) return;

        vx = (vx + (config.acceleration * deltaTime * dir)) * (1  - ((1 - config.deceleration) * deltaTime));

        if(vx > config.maxVx) vx = config.maxVx;
        if (vx < -config.maxVx) vx = -config.maxVx;

        vy -= jetpackThrust * deltaTime;
        vy = vy - config.gravityForce * deltaTime;

        x += vx * deltaTime;
        y += vy * deltaTime;

        if(x < 0){
            x = 0;
            vx = config.bounce ? -vx * config.bounceRestitution : 0;
            if(config.bounce && abs(vx) > 5){
                config.audioManager->playSFX("boing");
            }
        }

        if(x > config.screenWidth - collider.w){
            x = config.screenWidth - collider.w;
            vx = config.bounce ? -vx * config.bounceRestitution : 0;
            if(config.bounce && abs(vx) > 5){
                config.audioManager->playSFX("boing");
            }
        }

        if(y < 0){
            y = 0;
            vy = config.bounce ? -vy * config.bounceRestitution : 0;
            if(config.bounce && abs(vy) > 5){
                config.audioManager->playSFX("boing");
            }
        }

        if(y > config.screenHeight - collider.h){
            y = config.screenHeight - collider.h;
            vy = config.bounce ? -vy * config.bounceRestitution : 0;
            if(config.bounce && abs(vy) > 5){
                config.audioManager->playSFX("boing");
            }
        }

        jetpackThrust = 0.0f;

        dir = 0;

        collider.x = x;
        collider.y = y;


        if (life <= 0){
            isAlive = false;
            printf("Player dead\n");
        }
    }

    void Player::jetpack(){
        jetpackThrust = config.jetpackForce;

        if(thrustParticlesTimer.getTicks() >= 20){
            thrustParticlesTimer.start();
            config.particleManager->spawnThrustParticle(x + 5, y + 25, config.thrustParticleConfig);
        }
    }

    int Player::getScore(){
        return score;
    }

    void Player::handleInput(const Uint8* keys){
        if(!isAlive) return;

        if(config.joystickId != -1){
            SDL_Joystick* joystick = SDL_JoystickFromInstanceID(config.joystickId);
            handleJoystickInput(joystick);
        }else{
            if(keys[config.keyPreset.left]){
                move(-1);
            }
            if(keys[config.keyPreset.right]){
                move(1);
            }
            if(keys[config.keyPreset.thrust]){
                jetpack();
                if(!mJetpackActive){
                    jetpackChannel = config.audioManager->playSFX("jetpackThrust", -1);
                    mJetpackActive = true;
                }
            }else{
                if(mJetpackActive){
                    config.audioManager->stopChannel(jetpackChannel);
                    jetpackChannel = -1;
                    mJetpackActive = false;
                }
            }
            if(keys[config.keyPreset.missile]){
                if(config.ability){
                    config.ability->use(this);
                }
            }    
        }
    }

        

    void Player::handleJoystickInput(SDL_Joystick* joystick){
        if(!joystick) return;

        Sint16 axisX = SDL_JoystickGetAxis(joystick, 0);
        if(axisX < -DEAD_ZONE) move(-1);
        else if(axisX > DEAD_ZONE) move(1);

        if(SDL_JoystickGetButton(joystick, 0)){
            jetpack();
            if(!mJetpackActive){
                jetpackChannel = config.audioManager->playSFX("jetpackThrust", -1);
                mJetpackActive = true;
            }
        }else{
            if(mJetpackActive){
                config.audioManager->stopChannel(jetpackChannel);
                jetpackChannel = -1;
                mJetpackActive = false;
            }
        }

        if(SDL_JoystickGetButton(joystick, 1)){
            if(config.ability){
                config.ability->use(this);
            }
        };
    }

    int Player::getLife(){
        return life;
    }

    void Player::updateLife(int toAdd){
        if(isControlled) return;
        if(config.skinId == "skin_turtle"){
            toAdd*=0.75;
        };
        life+=toAdd;
    }

    LTexture* Player::getSkin() { return config.skin; }
    LTexture* Player::getHat() { return config.hat; }

    Player::~Player(){
        Mix_HaltChannel(jetpackChannel);
    }

    void Player::move(int direction){
        dir = direction;
    }

    void Player::applyKnockBack(float forceX, float forceY){
        vx += forceX;
        vy += forceY;
    }

    void Player::setVelocity(float vx, float vy){
        this->vx = vx;
        this->vy = vy;
    }

    std::string Player::getSkinId(){
        return config.skinId;
    }

    void Player::resolveCollisionWith(Player& other){
        if(isControlled || other.isControlled) return;
        float cx1 = collider.x + collider.w / 2.f;
        float cy1 = collider.y + collider.h / 2.f;
        float cx2 = other.collider.x + other.collider.w / 2.f;
        float cy2 = other.collider.y + other.collider.h / 2.f;

        float dx = cx1 - cx2;
        float dy = cy1 - cy2;

        float overlapX = (collider.w / 2.f + other.collider.w / 2.f) - std::abs(dx);
        float overlapY = (collider.h / 2.f+ other.collider.h / 2.f) - std::abs(dy);

        if(overlapX > 0 && overlapY > 0){
            float e = config.bounceRestitution;

            if(overlapX < overlapY){
                float push = overlapX / 2.f;
                if(dx > 0){
                    x+=push;
                    other.x -=push;
                }else{
                    x-=push;
                    other.x+=push;
                }

                float v1 = vx;
                float v2 = other.vx;
                vx = (v1 + v2 - e * (v1 - v2)) / 2.f;
                other.vx = (v1 + v2 + e*(v1 - v2)) / 2.f;
            }else{
                float push = overlapY / 2.f;
                if(dy > 0){
                    y+=push;
                    other.y -=push;
                }else{
                    y-=push;
                    other.y += push;
                }

                float v1 = vy;
                float v2 = other.vy;

                vy = (v1 + v2 - e * (v1 - v2)) / 2.f;
                other.vy = (v1 + v2 + e * (v1 - v2)) / 2.f;
            }

            collider.x = x;
            collider.y = y;
            other.collider.x = other.x;
            other.collider.y = other.y;
        }
    }

    int Player::getMaxLife(){ return config.maxHealth; }

    float Player::getAbilityProgress(){
        return config.ability->getCooldownProgress();
    }

    void Player::teleportTo(float x, float y){
        this->x = x;
        this->y = y;
    }
}
