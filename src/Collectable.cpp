#include "Collectable.hpp"

void Collectable::render(SDL_Renderer* renderer){
    cTexture->render(x, y);
}

void Collectable::setPos(float posX, float posY){
    x = posX;
    y = posY;
    collider.x = x;
    collider.y = y;
}
