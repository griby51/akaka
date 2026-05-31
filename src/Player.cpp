#include "Player.hpp"
#include "KeyPreset.hpp"
#include "Missile.hpp"
#include "Utils.hpp"
#include <SDL2/SDL_mixer.h>

namespace player{
    Player::Player(PlayerConfig& config) : config(config){
        isAlive = true;
        thrustParticlesTimer.start();
        currentThrustParticle = 0;
        missileTimer.start();

        collider.x = config.collider.x + x;
        collider.y = config.collider.y + y;
        collider.w = config.collider.w;
        collider.h = config.collider.h;

        for (int i = 0; i < 500; i++){
            thrustParticles[i].init(&config.thrustParticleConfig);
            thrustParticles[i].reset();
            thrustParticles[i].setPos(10000, 10000);
        }
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
        if(hat != nullptr){
            hat->render(x, y);
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
            vx = config.bounce ? -vx : 0;
        }

        if(x > config.screenWidth - collider.w){
            x = config.screenWidth - collider.w;
            vx = config.bounce ? -vx : 0;
        }

        if(y < 0){
            y = 0;
            vy = config.bounce ? -vy : 0;
        }

        if(y > config.screenHeight - collider.h){
            y = config.screenHeight - collider.h;
            vy = config.bounce ? -vy : 0;
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
        if(config.joystickId != -1){
            SDL_Joystick* joystick = SDL_JoystickFromInstanceID(config.joystickId);
            handleJoystickInput(joystick);
        };

        if(keys[config.keyPreset.left]){
            move(-1);
        }
        if(keys[config.keyPreset.right]){
            move(1);
        }
        if(keys[config.keyPreset.thrust]){
            jetpack();
            if(!mJetpackActive){
                jetpackChannel = Mix_PlayChannel(-1, config.jetpackSFX, -1);
                mJetpackActive = true;
            }
        }else{
            if(mJetpackActive){
                Mix_HaltChannel(jetpackChannel);
                jetpackChannel = -1;
                mJetpackActive = false;
            }
        }
        if(keys[config.keyPreset.missile]){
            spawnMissile();
        }
    }

    void Player::spawnMissile(){
        config.missileConfig.thrower = *this;

        SDL_Point p = util::spawnOffScreen(config.screenWidth, config.screenHeight, 200);

        config.missileManager->spawn(p.x, p.y, config.missileConfig);

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
                jetpackChannel = Mix_PlayChannel(-1, config.jetpackSFX, -1);
                mJetpackActive = true;
            }
        }else{
            if(mJetpackActive){
                Mix_HaltChannel(jetpackChannel);
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

    LTexture* Player::getSkin() { return skin; }
    LTexture* Player::getHat() { return hat; }

    Player::~Player(){
        Mix_HaltChannel(jetpackChannel);
    }
}