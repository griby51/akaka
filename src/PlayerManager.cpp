#include "PlayerManager.hpp"
#include "Utils.hpp"

namespace player{
    void PlayerManager::addPlayer(PlayerConfig&& config){
        players.emplace_back(std::move(config));
    }

    void PlayerManager::update(float deltaTime){
        const Uint8* keys = SDL_GetKeyboardState(NULL);
        for (auto& player : players) {
            player.update(deltaTime);
            player.handleInput(keys);
        }

        for(int i = 0; i < players.size(); i++){
            for(int j = i + 1; j < players.size(); j++){
                if(!players[i].isAlive || !players[j].isAlive) continue;

                if(util::collide(players[i].collider, players[j].collider)){
                    players[i].resolveCollisionWith(players[j]);
                }
            }
        }
    }

    void PlayerManager::render(SDL_Renderer* renderer){
        for (auto& player : players) {
            player.render(renderer);
        }
    }
}
