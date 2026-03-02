#include "CollisionSystem.hpp"

bool Collision::collide(SDL_Rect* collider1, SDL_Rect* collider2){
    SDL_Rect intersection;
    return SDL_IntersectRect(collider1, collider2, &intersection) == SDL_TRUE;
}