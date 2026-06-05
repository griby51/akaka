#pragma once

#include "Collectable.hpp"

class ScoreCollectable : public Collectable{
public:
    void init(int scoreOnHit, LTexture* texture);
    void update(float deltaTime, std::vector<player::Player>* players) override;
    void onHit(player::Player& player) override;
private:
    int cScore = 0;
};
