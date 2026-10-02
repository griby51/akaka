#include "World.hpp"

#include <algorithm>
#include <cstdlib>

void World::init(){
    context.events = &events;
    context.players = &playerManager.players;
    context.projectiles = &projectileManager;
    context.globalSpeed = &globalSpeed;
    context.screenWidth = &screenWidth;
    context.screenHeight = &screenHeight;
    context.effectiveHeight = &effectiveHeight;
}

void World::start(){
    mPizzaTimeUntilNext = rand() % 1000;
    mPizzaTimer.start();
}

void World::step(float deltaTime, const std::vector<PlayerInput>& inputs){
    scrollingOffset -= globalSpeed * deltaTime;
    if(scrollingOffset < -backgroundWidth){
        scrollingOffset = 0;
    }

    projectileManager.update(deltaTime);
    playerManager.update(deltaTime, inputs);

    pizzas.erase(
            std::remove_if(pizzas.begin(), pizzas.end(),
                [](const ScoreCollectable& col){
                return !col.isAlive;
                }),
            pizzas.end()
            );

    for(size_t i = 0; i < pizzas.size(); i++){
        pizzas[i].update(deltaTime, &playerManager.players);
    }

    if(mPizzaTimer.getTicks() > mPizzaTimeUntilNext){
        mPizzaTimeUntilNext = rand() % 1000;
        mPizzaTimer.start();
        pizzas.emplace_back();
        pizzas.back().init(100, "pizza");
        pizzas.back().setPos(screenWidth, rand() % (effectiveHeight - 16));
        pizzas.back().vx = -globalSpeed * 10;
        pizzas.back().collider.w = 16;
        pizzas.back().collider.h = 16;
    }

    tick++;
}
