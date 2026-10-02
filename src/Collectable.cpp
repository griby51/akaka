#include "Collectable.hpp"

void Collectable::setPos(float posX, float posY){
    x = posX;
    y = posY;
    collider.x = x;
    collider.y = y;
}
