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

        void triggerShake(float intensity, float duration);

        int getShakeX() const { return shakeX; }
        int getShakeY() const { return shakeY; }

    private:
        std::vector<Explosion> mExplosions;

        float shakeIntensity = 0.f;
        float shakeDuration = 0.f;
        int shakeX = 0;
        int shakeY = 0;
    };
}
