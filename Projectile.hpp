#pragma once

#include <SDL2/SDL.h>

class Projectile{
    public:
        Projectile();
        void update();

        int getX(), getY(), getVelX(), getVelY();

        void setPos(int posX, int posY);
        void setVelocity(int vx, int vy);

        void setCollider(int colX, int colY, int colW, int colH);

        void drawCollider(SDL_Renderer* renderer, SDL_Color* color);

        SDL_Rect collider;

    private:
        int x, y;
        int vx, vy;

        int distanceBetweenColliderXAnd0;
        int distanceBetweenColliderYAnd0;
};