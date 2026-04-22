#pragma once
#include <SDL2/SDL.h>
#include "Config.hpp"
#include "LTimer.hpp"
#include "LTexture.hpp"
#include "Particle.hpp"
#include "KeyPreset.hpp"
#include "Missile.hpp"

class Player{
    public:
        void init(GameConfig* config, int _index);

        void render(SDL_Renderer* renderer);
        void setPos(float posX, float posY);
        void move(int direction);
        void jetpack();

        void setScreenSize(int width, int height);

        void updateScore(int toAdd);
        void setCollider(int colX, int colY, int colW, int colH);

        void drawCollider(SDL_Renderer* renderer, SDL_Color* color);

        int getScore();

        void setKeyPreset(KeyPreset preset);
        
        void handleInput(const Uint8* keys);

        void update(float deltaTime);
        void setSkin(LTexture* _skin);
        void setHat(LTexture* _hat);

        int getX();
        int getY();
        
        void setParticleConfig(GameConfig config);

        void setMissileTable(Missile* _missiles, int tableSize, int* _currentMissile);
        void setPlayerTable(Player* _players, int tableSize);

        void spawnMissile();

        SDL_Rect collider;

    private:
        float x, y; 
        float vx, vy;
        int score;
        int dir;

        int index;

        LTexture* skin;
        LTexture* hat;

        GameConfig* g_config;
        int distanceBetweenColliderXAnd0;
        int distanceBetweenColliderYAnd0;

        float jetpackThrust = 0.0f;

        float JETPACK_FORCE;
        float MAX_VY;
        float MAX_VX;
        float ACCELERATION;
        float DECELERATION;
        float GRAVITY_FORCE;
        float BOUNCE_RESTITUTION; 
        bool BOUNCE;

        KeyPreset mPreset;

        int SCREEN_WIDTH, SCREEN_HEIGHT;

        LTimer thrustParticlesTimer;
        ThrustParticle thrustParticles[500];
        ParticleConfig thrustParticleConfig;
        int currentThrustParticle;
        
        int* currentMissile;

        Missile* missiles;
        int missileTableSize;

        Player* players;
        int playerTableSize;

        LTimer missileTimer;
        int scoreToLaunchMissile;
};
