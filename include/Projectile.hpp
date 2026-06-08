#pragma once

#include <SDL2/SDL.h>

namespace projectile{
    class Projectile{
    public:
        virtual void update(float deltaTime) = 0;
        virtual void render(SDL_Renderer* renderer) = 0;
        virtual ~Projectile() = default;

        bool isDead() const {return !isAlive; }

    protected:
        float x, y;
        float vx, vy;
        bool isAlive = false;
        SDL_Rect collider;
    };
}
