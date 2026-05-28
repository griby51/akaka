#pragma once
#include <SDL2/SDL.h>

namespace explode {
    class BaseParticle{
    public:
        BaseParticle(float x, float y, float vx, float vy, float lifetime) :
            mX(x), mY(y), mVx(vx), mVy(vy), mLifetime(lifetime), mAge(0.f) {}

        virtual ~BaseParticle() = default;

        virtual void update(float dt) = 0;
        virtual void render(SDL_Renderer* renderer) const = 0;

        bool isDead() const {
            return mAge >= mLifetime;
        }

        float progress() const {
            return mAge / mLifetime;
        }

    protected:
        float mX, mY;
        float mVx, mVy;
        float mLifetime;
        float mAge;
    };
}
