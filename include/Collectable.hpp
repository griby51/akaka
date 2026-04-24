#pragma once

#include <SDL2/SDL.h>
#include <vector>

#include "LTexture.hpp"
#include "Player.hpp"
#include "CollisionSystem.hpp"

class Collectable{
public:
    void render(SDL_Renderer* renderer);
    void setPos(float posX, float posY);

    virtual void update(float deltaTime, std::vector<Player>* players) = 0;
    virtual void onHit(Player& player) = 0;

    float x, y;
    float vx, vy;
    SDL_Rect collider;
    bool isAlive = true;
protected:
    LTexture* cTexture;
};
