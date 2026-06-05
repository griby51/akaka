#include "EmberParticle.hpp"
#include "BaseParticle.hpp"
#include "util.hpp"
#include <SDL2/SDL_stdinc.h>

namespace explode {
    EmberParticle::EmberParticle(float x, float y, float vx,  float vy, float lifetime):
        BaseParticle(x, y, vx, vy, lifetime),
        mTrailTimer(0.f),
        mTrailFadeTimer(0.f)
    {}

    void EmberParticle::update(float dt){
        
        if(mAge >= mLifetime){
            mTrailFadeTimer += dt;
            float fadeMax = TRAIL_LEN * 0.05f;
            if(mTrailFadeTimer >= fadeMax){
                mTrail.clear();
            }
            return;
        }

        mAge += dt;
        mTrailTimer += dt;

        if(mTrailTimer >= TRAIL_STEP){
            mTrailTimer = 0.f;
            mTrail.push_back({
                    mX, mY, mAge
                });
            if((int)mTrail.size() > TRAIL_LEN){
                mTrail.erase(mTrail.begin());
            }
        }

        mVy += 320.f * dt;
        mVx *= (1.f - 0.8f * dt);
        mX += mVx * dt;
        mY += mVy * dt;
    }

    void EmberParticle::render(SDL_Renderer* renderer) const {
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

        float fadeFactor = 1.f;

        if(mAge >= mLifetime){
            float fadeMax = TRAIL_LEN * 0.05f;
            fadeFactor = 1.f - std::min(mTrailFadeTimer / fadeMax, 1.f);
        }

        for(int i = 0; i < (int)mTrail.size(); i++){
            float tf = (float)i / (float)TRAIL_LEN;
            SDL_Color c0 = {255, 255, 200, 0};
            SDL_Color c1 = {255, 200, 50, 180};
            SDL_Color c2 = {255, 255, 240, 240};

            SDL_Color col = util::gradient3(c0, c1, c2, tf);

            col.a = (Uint8)(col.a * fadeFactor);

            SDL_SetRenderDrawColor(renderer, col.r, col.g, col.b, col.a);
            SDL_RenderDrawPoint(renderer, (int)mTrail[i].x, (int)mTrail[i].y);

            SDL_RenderDrawPoint(renderer, (int)mTrail[i].x + 1, (int)mTrail[i].y);
            SDL_RenderDrawPoint(renderer, (int)mTrail[i].x, (int)mTrail[i].y + 1);
        }

        float t = progress();

        if(mAge < mLifetime){
            Uint8 alpha = (Uint8)(255 * (1.f - t));
            SDL_SetRenderDrawColor(renderer, 255, 240, 180, alpha);
            SDL_RenderDrawPoint(renderer, (int)mX, (int)mY);
            SDL_RenderDrawPoint(renderer, (int)mX + 1, (int)mY);
            SDL_RenderDrawPoint(renderer, (int)mX + 1, (int)mY + 1);
            SDL_RenderDrawPoint(renderer, (int)mX, (int)mY + 1);    
        }
    }

    bool EmberParticle::isDead() const{
        return mAge >= mLifetime && mTrail.empty();
    }
}
