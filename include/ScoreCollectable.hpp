#pragma once
#include <string>

#include "Collectable.hpp"

class ScoreCollectable : public Collectable{
public:
    void init(int scoreOnHit, std::string textureId);
    void update(float deltaTime, std::vector<player::Player>* players) override;
    void onHit(player::Player& player) override;
private:
    int cScore = 0;
};
