#pragma once
#include <SDL2/SDL.h>
#include <algorithm>
#include <vector>
#include <memory>
#include "BaseParticle.hpp"

namespace explode{
    struct ExplosionConfig{
        float power = 1.0f;
        float groundY = -1;

        bool fireballs = true;
        bool debris = true;
        bool embers = true;
        bool smoke = true;
    };

    class Explosion{
    public:
        Explosion(float x, float y, const ExplosionConfig& cfg = {});

        void update(float dt);
        void render(SDL_Renderer* renderer) const;
        bool isDone() const;

    private:
        std::vector<std::unique_ptr<BaseParticle>> mParticles;
    };
}
