#include "Projectile.hpp"

Projectile::Projectile(){
    x = 0;
    y = 0;
    vx = 0;
    vy = 0;
    collider.x = 0;
    collider.y = 0;
    collider.w = 0;
    collider.h = 0;
}

int Projectile::getX(){return x;}
int Projectile::getY(){return y;}
int Projectile::getVelX(){return vx;}
int Projectile::getVelY(){return vy;}

void Projectile::update(){
    x+=vx;
    y+=vy;
    collider.x = x + distanceBetweenColliderXAnd0;
    collider.y = y + distanceBetweenColliderYAnd0;
}

void Projectile::setPos(int posX, int posY){
    x = posX;
    y = posY;
}

void Projectile::setVelocity(int velocityX, int velocityY){
    vx = velocityX;
    vy = velocityY;
}

void Projectile::setCollider(int colX, int colY, int colW, int colH){
    distanceBetweenColliderXAnd0 = colX;
    distanceBetweenColliderYAnd0 = colY;
    colX += x;
    colY += y;
    collider.x = colX;
    collider.y = colY;
    collider.w = colW;
    collider.h = colH;
}

void Projectile::drawCollider(SDL_Renderer* renderer, SDL_Color* color){
    SDL_SetRenderDrawColor(renderer, color->r, color->g, color->b, color->a);
    SDL_RenderDrawRect(renderer, &collider);
}
