#pragma once

#include <string>

struct PlayerInfo{
    std::string skinId;
    std::string hatId;
    int maxLife = 100;
    int colliderW = 32;
    int colliderH = 32;
    bool showCollider = false;
};
