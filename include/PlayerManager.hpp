#pragma once

#include "Player.hpp"
#include "PlayerInput.hpp"
#include <vector>

namespace player{
    class PlayerManager{
    public:
        void update(float deltaTime, const std::vector<PlayerInput>& inputs);
        void addPlayer(PlayerConfig&& config);
        std::vector<Player> players;
    };
}
