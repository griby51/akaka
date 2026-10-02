#pragma once

#include "Player.hpp"
#include "PlayerInput.hpp"
#include <vector>

namespace player{
    class PlayerManager{
    public:
        void update(float deltaTime, const std::vector<PlayerInput>& inputs);
        void render(SDL_Renderer* renderer);
        void addPlayer(PlayerConfig&& config);
        std::vector<Player> players;
    };
}
