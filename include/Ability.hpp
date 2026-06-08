#pragma once

namespace player {class Player; }

class Ability{
public:
    virtual ~Ability() = default;

    virtual void use(player::Player* player) = 0;

    virtual void update(float deltaTime) {}
};
