#pragma once
#include "Missile.hpp"
#include <vector>

namespace missile{
    class MissileManager{
    public:
        void spawn(float x, float y, MissileConfig cfg);
        void update(float deltaTime);
        void render(SDL_Renderer* renderer);
    private:
        std::vector<Missile> missiles;
    };
}
