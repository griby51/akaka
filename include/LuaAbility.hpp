#pragma once

#include "Ability.hpp"
#include <sol/sol.hpp>

struct GameContext;

class LuaAbility : public Ability{
public:
    LuaAbility(sol::table instance, GameContext* ctx);
    void use(player::Player* player) override;
private:
    sol::table instance;
    sol::protected_function onUse;
    GameContext* ctx;
};
