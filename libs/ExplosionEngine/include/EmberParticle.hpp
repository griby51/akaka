#pragma once
#include "BaseParticle.hpp"
#include <vector>

namespace explode {
    class EmberParticle : public BaseParticle{
    public:
        EmberParticle(float x, float y, float vx, float vy, float lifetime);

        void update(float dt) override;
        void render(SDL_Renderer* renderer) const override;

        bool isDead() const;

    private:
        struct TrailPoint {
            float x, y;
            float age;
        };
        std::vector<TrailPoint> mTrail;
        static constexpr int TRAIL_LEN = 12;
        static constexpr float TRAIL_STEP = 0.016f;
        float mTrailTimer;
        float mTrailFadeTimer;
    };
}
