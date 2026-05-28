#pragma once
#include "Explosion.hpp"
#include <vector>

namespace explode{
    class ExplosionManager{
    public:
        void spawn(float x, float y, const ExplosionConfig& cfg = {});
        void update(float dt);
        void render(SDL_Renderer* renderer) const;
        int count() const {
            return (int)mExplosions.size();
        }

    private:
        std::vector<Explosion> mExplosions;
    };
}
