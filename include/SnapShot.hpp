#pragma once

#include <cstdint>
struct PlayerState{
    uint8_t index = 0;
    float x = 0.f, y = 0.f;
    float vx = 0.f, vy = 0.f;
    int16_t life = 0;
    int32_t score = 0;
    bool isAlive = false;
    bool isControlled = false;
    bool thrusting = false;
};
