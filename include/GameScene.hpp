#pragma once
#include "Scene.hpp"
#include "SceneManager.hpp"
#include "Game.hpp"
#include "PlayerSlot.hpp"

class GameScene : public Scene{
public:
    GameScene(SDL_Renderer* renderer, SDL_Window* window, SceneManager& manager, PlayerSlot* slots, int joinedCount);

    void handleEvent(const SDL_Event& e) override;
    void update(float deltaTime) override;
    void render() override;
    bool isDone() override;

private:
SceneManager& mManager;
    Game mGame;
    bool mDone = false;
};
