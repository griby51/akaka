#include "Explosion.hpp"
#include "FireballParticle.hpp"
#include "EmberParticle.hpp"
#include "util.hpp"

#include <cstdlib>
#include <cmath>
#include <memory>

namespace explode{
    Explosion::Explosion(float x, float y, const ExplosionConfig& cfg){
        float power = cfg.power;

        if(cfg.fireballs){

            int nFlash = (int)(60 * power);
            for (int i = 0; i < nFlash; i++){
                float a = util::rAngle();
                float speed = util::rF(10.f, 180.f) * power;
                mParticles.push_back(std::make_unique<FireballParticle>(
                            x + util::rF(-6.f, 6.f), y + util::rF(-6.f, 6.f),
                            std::cos(a) * speed,
                            std::sin(a) * speed - util::rF(0.f, 60.f),
                            util::rF(2.f, 7.f),
                            0.f,
                            util::rF(0.12f, 0.30f)
                ));
            }
        }

        if(cfg.embers){
            int nEmbers = (int)(45 * power);
            for (int i = 0; i < nEmbers; i++){
                float a = util::rAngle();
                float speed = util::rF(50.f, 250.f) * power;

                mParticles.push_back(std::make_unique<EmberParticle>(
                            x, y,
                            std::cos(a) * speed,
                            std::sin(a) * speed - util::rF(20.f, 100.f),
                            util::rF(0.5f, 1.2f)
                ));
            }
        }
    }

    void Explosion::update(float dt){
        for (auto& p : mParticles){
            p->update(dt);
        }
    }

    void Explosion::render(SDL_Renderer* renderer) const {
        for(auto& p : mParticles){
            p->render(renderer);
        }
    }

    bool Explosion::isDone() const {
        for (auto& p : mParticles){
            if(!p->isDead()) return false;
        }
        return true;
    }
}
