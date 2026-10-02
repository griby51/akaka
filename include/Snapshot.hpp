#pragma once

#include <cstdint>
#include <vector>

#include "GameEvent.hpp"

class World;

struct PlayerState{
    uint8_t index = 0;
    float x = 0.f, y = 0.f;
    float vx = 0.f, vy = 0.f;
    int16_t life = 0;
    int32_t score = 0;
    bool isAlive = false;
    bool isControlled = false;
    bool thrusting = false;
    uint8_t abilityProgress = 0;
};

struct EntityState{
    uint32_t netId = 0;
    uint16_t texture = 0;
    float x = 0.f, y = 0.f;
    float angle = 0.f;
    uint16_t w = 0, h = 0;
};

struct Snapshot{
    uint32_t tick = 0;
    float scrollingOffset = 0.f;
    std::vector<PlayerState> players;
    std::vector<EntityState> projectiles;
    std::vector<EntityState> collectables;
    std::vector<GameEvent> events;
};

Snapshot captureSnapshot(const World& world);
