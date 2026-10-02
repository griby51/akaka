#pragma once
#include <vector>

namespace player{class Player; }
namespace projectile{class ProjectileManager; }
class ParticleManager;
class EventQueue;

struct GameContext{
    std::vector<player::Player>* players = nullptr;
    projectile::ProjectileManager* projectiles = nullptr;
    EventQueue* events = nullptr;
    ParticleManager* particleManager = nullptr;
    int* screenWidth = nullptr;
    int* effectiveHeight = nullptr;
    int* screenHeight = nullptr;
    float* globalSpeed = nullptr;
};

