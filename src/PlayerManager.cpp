#include "PlayerManager.hpp"

namespace player{
    void PlayerManager::addPlayer(PlayerConfig& config){
        Player player(config);
        players.push_back(player);
    }

    void PlayerManager::update(float deltaTime){
        const Uint8* keys = SDL_GetKeyboardState(NULL);
        for (auto& player : players) {
            player.update(deltaTime);
            player.handleInput(keys);
        }
    }

    void PlayerManager::render(SDL_Renderer* renderer){
        for (auto& player : players) {
            player.render(renderer);
        }
    }
}