#include "Player.hpp"
#include "KeyPreset.hpp"
#include "Missile.hpp"
#include "Utils.hpp"
#include <SDL2/SDL_mixer.h>

namespace player{
    Player::Player(PlayerConfig config) : config(config){
        isAlive = true;
        thrustParticlesTimer.start();
        currentThrustParticle = 0;
        missileTimer.start();

        x = config.screenWidth / 2.0f;
        y = config.screenHeight / 4.0f;

        vx = 0.f;
        vy =0.f;
        dir = 0;

        collider.x = config.collider.x + x;
        collider.y = config.collider.y + y;
        collider.w = config.collider.w;
        collider.h = config.collider.h;


        for (int i = 0; i < 500; i++){
            thrustParticles[i].init(&this->config.thrustParticleConfig);
        }

        missileTimer.start();

        life = config.maxHealth;
        score = 0;
    }

    void Player::updateScore(int toAdd){
        score += toAdd;
    }

    void Player::render(SDL_Renderer* renderer){
        for(int i = 0; i < 500; i++){
            thrustParticles[i].render(renderer);
        }

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

        for (int i = 0; i < 500; i++){
            thrustParticles[i].update(deltaTime);
        }

        if(!isAlive) return;

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
            thrustParticles[currentThrustParticle].reset();
            thrustParticles[currentThrustParticle].setPos(x + 5, y + 25);
            currentThrustParticle = (currentThrustParticle + 1) % 500;
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
                spawnMissile();
            }    
        }
    }

    void Player::spawnMissile(){
        if(missileTimer.getTicks() < 300) return;
        if(score < config.scoreToLaunchMissile) return;

        missileTimer.start();

        int myIndex = -1;
        if(config.missileConfig.players){
            for(int i = 0; i < config.missileConfig.players->size(); i++){
                if(&(*config.missileConfig.players)[i] == this){
                    myIndex = i;
                    break;
                }
            }
        }
        
        missile::MissileConfig cfg = config.missileConfig;
        cfg.throwerIndex = myIndex;

        score -= config.scoreToLaunchMissile;

        SDL_Point p = util::spawnOffScreen(config.screenWidth, config.screenHeight, 200);

        config.missileManager->spawn(p.x, p.y, cfg);

        missileTimer.start();
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

        if(SDL_JoystickGetButton(joystick, 1)) spawnMissile();
    }

    int Player::getLife(){
        return life;
    }

    void Player::updateLife(int toAdd){
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
}
