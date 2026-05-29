#include "ScoreCollectable.hpp"
#include "Utils.hpp"

void ScoreCollectable::init(int scoreOnHit, LTexture* texture){
    cTexture = texture;
    cScore = scoreOnHit;
}

void ScoreCollectable::update(float deltaTime, std::vector<Player>* players){
    x += vx * deltaTime;
    y += vy * deltaTime;
    collider.x = x;
    collider.y = y;

    for(int i = 0; i < players->size(); i++){
        if(util::collide((*players)[i].collider, collider)){
            onHit((*players)[i]);
        }
    }
}

void ScoreCollectable::onHit(Player& player){
    player.updateScore(cScore);
    isAlive = false;
}

