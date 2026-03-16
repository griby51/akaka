#pragma once
#include <SDL2/SDL.h>
#include "Config.hpp"

class Player{
    public:
        void init(GameConfig* config);

        void render();
        void setPos(float posX, float posY);
        void move(int direction);
        void jetpack();

        void setScreenSize(int width, int height);

        void updateScore(int toAdd);
        void setCollider(int colX, int colY, int colW, int colH);

        void drawCollider(SDL_Renderer* renderer, SDL_Color* color);

        int getScore();

        void update(float deltaTime);

        int getX();
        int getY();
        SDL_Rect collider;
    
    private:
        float x, y; 
        float vx, vy;
        int score;
        int dir;

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

        int SCREEN_WIDTH, SCREEN_HEIGHT;
};
