#pragma once
#include <SDL2/SDL.h>


class Player{
    public:
        Player();

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

        int distanceBetweenColliderXAnd0;
        int distanceBetweenColliderYAnd0;

        float jetpackThrust = 0.0f;

        const float JETPACK_FORCE = 1000.0f;
        const float MAX_VY = 100.0f;
        const float MAX_VX = 1000.0f;
        const float acceleration = 1000.0f;
        const float deceleration = 0.50f;
        const float GRAVITY_FORCE = -500.0f;
        const float BOUNCE_RESTITUTION = 0.8f; 

        int SCREEN_WIDTH, SCREEN_HEIGHT;
};
