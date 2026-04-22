#include "GameScene.hpp"
#include "PlayerSlot.hpp"

GameScene::GameScene(SDL_Renderer* renderer, SDL_Window* window, SceneManager& manager, PlayerSlot* playerSlots, int joinedCount)
    : mManager(manager)
{
    mGame = new Game();
    

    mGame->init(renderer, window, playerSlots, joinedCount);
    mGame->loadMedia();
    mGame->start();
}

void GameScene::handleEvent(const SDL_Event& e){
    if(e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE){
        mManager.pop();
        return;
    }
    mGame->handleEvents(e);
}

void GameScene::update(float deltaTime){
    if(mGame->isOver()){
        mDone = true;
        return;
    }
    mGame->update(deltaTime);
    mGame->handleInput();
}

void GameScene::render(){
    mGame->render();
}

bool GameScene::isDone(){
    return mDone;
}
