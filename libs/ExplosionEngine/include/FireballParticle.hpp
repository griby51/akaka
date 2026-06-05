#pragma once
#include "BaseParticle.hpp"

namespace explode{
    class FireballParticle : public BaseParticle{
    public:
        FireballParticle(float x, float y, float vx, float vy, float startRadius, float endRadius, float lifetime);
        void update(float dt) override;
        void render(SDL_Renderer* renderer) const override;
    private:
        float mStartRadius;
        float mEndRadius;
    };
}
