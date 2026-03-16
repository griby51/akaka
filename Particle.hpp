#ifndef PARTICLE_HPP
#define PARTICLE_HPP

#include "SDL2/SDL.h"
#include <iostream>
#include <cmath>
#include <random>
#include <algorithm>

class Particle{
public:
    SDL_Rect rect;

    SDL_Renderer* renderer;

    float posX, posY;

    int speed;

    int velocityX;
    int velocityY;

    int direction;

    float life;
    float lifeMax;

    SDL_Color startColor;
    SDL_Color endColor;

    SDL_Color color;

    Particle(int x, int y, int w, int h, int minSpeed, int maxSpeed, int minAngle, int maxAngle, float lifeTime, SDL_Color colorStart, SDL_Color colorEnd, SDL_Renderer* _renderer);
    void update(float deltaTime);
    bool isAlive();
    void render();
};

#endif