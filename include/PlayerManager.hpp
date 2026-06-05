#pragma once

#include "Player.hpp"
#include <vector>

namespace player{
    class PlayerManager{
    public:
        void update(float deltaTime);
        void render(SDL_Renderer* renderer);
        void addPlayer(PlayerConfig& config);
        std::vector<Player> players;
    };
}