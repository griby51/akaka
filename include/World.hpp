#pragma once

#include <cstdint>
#include <vector>

#include "GameContext.hpp"
#include "GameEvent.hpp"
#include "LTimer.hpp"
#include "PlayerInput.hpp"
#include "PlayerManager.hpp"
#include "ProjectileManager.hpp"
#include "ScoreCollectable.hpp"

class World{
public:
    void init();
    void start();
    void step(float deltaTime, const std::vector<PlayerInput>& inputs);

    player::PlayerManager playerManager;
    projectile::ProjectileManager projectileManager;
    std::vector<ScoreCollectable> pizzas;

    EventQueue events;
    GameContext context;

    float globalSpeed = 50.0f;
    float scrollingOffset = 0.f;
    uint32_t tick = 0;

    int screenWidth = 800;
    int screenHeight = 600;
    int effectiveHeight = 550;

    float backgroundWidth = 0.f;

private:
    LTimer mPizzaTimer;
    int mPizzaTimeUntilNext = 0;
};
