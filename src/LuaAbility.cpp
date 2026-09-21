#include "LuaAbility.hpp"
#include "GameContext.hpp"
#include "Player.hpp"
#include <sol/error.hpp>
#include <sol/optional_implementation.hpp>
#include <sol/protected_function_result.hpp>

LuaAbility::LuaAbility(sol::table instance, GameContext* ctx)
    : instance(instance), ctx(ctx){
        onUse = instance["onUse"];
        cost = instance.get_or("cost", 0);
        cooldown = (int)(instance.get_or("cooldown", 0.0) * 1000.0);
        timeSinceLast.start();
    }

void LuaAbility::use(player::Player* player){
    if(timeSinceLast.getTicks() <= cooldown) return;
    if(player->getScore() < cost) return;
    sol::protected_function_result result = onUse(instance, player, ctx);

    if(!result.valid()){
        sol::error err = result;
        printf("Ability error : %s\n", err.what());
        return;
    }

    sol::optional<bool> used = result;
    if(!used.value_or(false)) return;

    player->updateScore(-cost);
    timeSinceLast.start();
}
