#include "FireballParticle.hpp"
#include "BaseParticle.hpp"
#include "util.hpp"
#include <SDL2/SDL_blendmode.h>
#include <SDL2/SDL_render.h>

namespace explode{
    FireballParticle::FireballParticle(float x, float y, float vx, float vy, float startRadius, float endRadius, float lifetime)
        : BaseParticle(x, y, vx, vy, lifetime),
        mStartRadius(startRadius), mEndRadius(endRadius) {}

    void FireballParticle::update(float dt){
        mAge += dt;
        mVx *= (1.f - 1.5f * dt);
        mVy -= 30.f * dt;
        mX += mVx * dt;
        mY += mVy * dt;
    }

    void FireballParticle::render(SDL_Renderer* renderer) const {
        float t = progress();

        float radius = mStartRadius * (1 - t * 0.5f);
        if (radius < 1.f) radius = 1.f;

        SDL_Color c0 = {255, 220, 120, 255};
        SDL_Color c1 = {255, 90, 10, 220};
        SDL_Color c2 = {140, 10, 0, 0};

        SDL_Color col = util::gradient3(c0, c1, c2, t);

        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, col.r, col.g, col.b, col.a);
        util::fillCircle(renderer, (int)mX, (int)mY, (int)radius);
    }
}
