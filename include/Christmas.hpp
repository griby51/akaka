#pragma once
#include "Explosion.hpp"
#include "Particle.hpp"
#include "ExplosionManager.hpp"
#include "AudioManager.hpp"
#include "LTimer.hpp"
#include "ParticleManager.hpp"
#include "Projectile.hpp"

#include <SDL2/SDL.h>
#include <vector>

namespace player{class Player;}

namespace projectile{

    struct GiftConfig{
        std::vector<player::Player>* players = nullptr;
    }
    struct ChristmasSleighConfig{
        std::vector<player::Player>* players = nullptr;

        int throwerIndex = -1;
        float speed = 2000.f;
        
        std::string textureId = "christmasSleigh";
        
        explode::ExplosionManager* explosionManager;
        explode::ExplosionConfig explosionConfig;
        
        AudioManager* audioManager;
    };
};