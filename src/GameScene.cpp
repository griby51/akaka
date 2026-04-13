#include "GameScene.hpp"

GameScene::GameScene(SDL_Renderer* renderer, SDL_Window* window,SceneManager& manager)
    : mManager(manager)
{
    mGame.init(renderer, window);
    mGame.start();
    mGame.loadMedia();
}

void GameScene::handleEvent(const SDL_Event& e){
    if(e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE){
        mManager.pop();
        return;
    }
    mGame.handleEvents(e);
}

void GameScene::update(float deltaTime){
    if(mGame.isOver()){
        mDone = true;
        return;
    }
    mGame.update(deltaTime);
    mGame.handleInput();
}

void GameScene::render(){
    mGame.render();
}

bool GameScene::isDone(){
    return mDone;
}
