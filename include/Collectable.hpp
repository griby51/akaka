#pragma once

#include <SDL2/SDL.h>
#include <cstdint>
#include <vector>

#include "LTexture.hpp"
#include "Player.hpp"
#include "Utils.hpp"

class Collectable{
public:
    void setPos(float posX, float posY);

    virtual void update(float deltaTime, std::vector<player::Player>* players) = 0;
    virtual void onHit(player::Player& player) = 0;

    float x, y;
    float vx, vy;
    uint16_t textureAssetId = 0;
    SDL_Rect collider;
    bool isAlive = true;
protected:
    LTexture* cTexture;
};
