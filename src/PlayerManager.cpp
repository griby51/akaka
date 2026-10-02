#include "PlayerManager.hpp"
#include "Utils.hpp"

namespace player{
    void PlayerManager::addPlayer(PlayerConfig&& config){
        players.emplace_back(std::move(config));
    }

    void PlayerManager::update(float deltaTime, const std::vector<PlayerInput>& inputs){
        for(size_t i = 0; i < players.size(); i++){
            if(i < inputs.size()) players[i].applyInput(inputs[i]);
            players[i].update(deltaTime);
        }

        for(size_t i = 0; i < players.size(); i++){
            for(size_t j = i + 1; j < players.size(); j++){
                if(!players[i].isAlive || !players[j].isAlive) continue;

                if(util::collide(players[i].collider, players[j].collider)){
                    players[i].resolveCollisionWith(players[j]);
                }
            }
        }
    }

}
