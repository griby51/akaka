#pragma once
#include <vector>

namespace player{class Player; }
namespace projectile{class ProjectileManager; }
class ParticleManager;
class EffectManager;
class AudioManager;

struct GameContext{
    std::vector<player::Player>* players = nullptr;
    projectile::ProjectileManager* projectiles = nullptr;
    AudioManager* audioManager = nullptr;
    EffectManager* effectManager = nullptr;
    ParticleManager* particleManager = nullptr;
    int* screenWidth = nullptr;
    int* effectiveHeight = nullptr;
    int* screenHeight = nullptr;
};

