#pragma once
#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>

#include "Config.hpp"
#include "LTimer.hpp"
#include "LTexture.hpp"
#include "Particle.hpp"
#include "KeyPreset.hpp"
#include "MissileManager.hpp"

namespace player{
    struct PlayerConfig{
        float jetpackForce = 700.0f;
        float maxVy = 600.0f;
        float maxVx = 1000.0f;
        float acceleration = 1000.0f;
        float deceleration = 0.50f;
        float gravityForce = -500.0f;
        float bounceRestitution = 0.4f;

        int maxHealth = 100;

        int scoreToLaunchMissile = 200;
        bool bounce = false;

        int screenWidth = 800;
        int screenHeight = 600;

        bool showCollider = false;
        SDL_Rect collider = {0, 0, 32, 32};
        SDL_Color colliderColor = {255, 0, 255, 255};

        KeyPreset keyPreset;
        int joystickId = -1;

        LTexture* skin;
        LTexture* hat;

        missile::MissileManager* missileManager;
        missile::MissileConfig missileConfig;

        std::vector<Player>* players;

        ParticleConfig thrustParticleConfig;

        Mix_Chunk* jetpackSFX = NULL;
    };

    class Player{
    public:
        Player(PlayerConfig config);
        ~Player();

        void render(SDL_Renderer* renderer);
        void move(int direction);
        void jetpack();

        void updateScore(int toAdd);

        int getScore();
        
        void handleInput(const Uint8* keys);
        void handleJoystickInput(SDL_Joystick* joystick);

        void setJoystickId(int id);
        int getJoystickId();

        void update(float deltaTime);
        
        void spawnMissile();

        void applyKnockBack(float forceX, float forceY);

        void updateLife(int toAdd);
        int getLife();

        bool isAlive;

        LTexture* getSkin();
        LTexture* getHat();

        SDL_Rect collider;

    private:
        float x, y; 
        float vx, vy;
        int score;
        int dir;

        static constexpr int DEAD_ZONE = 8000;

        LTexture* skin;
        LTexture* hat;

        float jetpackThrust = 0.0f;

        LTimer thrustParticlesTimer;
        ThrustParticle thrustParticles[500];
        ParticleConfig thrustParticleConfig;

        int currentThrustParticle;

        int missileTableSize;

        Player* players;
        int playerTableSize;

        LTimer missileTimer;
        int scoreToLaunchMissile;
        
        int life;
        int jetpackChannel = -1;
        bool mJetpackActive = false;

        PlayerConfig config;
    };
}

