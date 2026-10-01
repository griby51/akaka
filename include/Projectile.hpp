#pragma once

#include <SDL2/SDL.h>

namespace projectile{
    class Projectile{
    public:
        virtual void update(float deltaTime) = 0;
        virtual void render(SDL_Renderer* renderer) = 0;
        virtual ~Projectile() = default;

        bool isDead() const {return !isAlive; }
        void kill(){ isAlive = false; }

    protected:
        float x = 0.f, y = 0.f;
        float vx = 0.f, vy = 0.f;
        bool isAlive = true;
        SDL_Rect collider = {0, 0, 0, 0};
    };
}
