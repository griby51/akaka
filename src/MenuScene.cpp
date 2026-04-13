#include "MenuScene.hpp"
#include "GameScene.hpp"
#include <SDL2/SDL_mouse.h>

MenuScene::MenuScene(SDL_Renderer* renderer, SDL_Window* window ,SceneManager& manager):
    mManager(manager)
{
    mRenderer = renderer;
    mWindow = window;
    SDL_GetRendererOutputSize(mRenderer ,&mScreenWidth, &mScreenHeight);
    playBtnHitbox = {mScreenWidth / 2 - 50, mScreenHeight / 2 - 10, 100, 20}; 
    mPlayBtnTexture.setRenderer(mRenderer);
    mPlayBtnTexture.loadFromeFile("assets/buttons/playBtn.png");
}

void MenuScene::update(float deltaTime){}

MenuScene::~MenuScene(){
    if(mFont) TTF_CloseFont(mFont);
}

void MenuScene::render(){
    SDL_SetRenderDrawColor(mRenderer, 10, 10, 20, 255);
    SDL_RenderClear(mRenderer);
    SDL_SetRenderDrawColor(mRenderer, 255, 0, 0, 255);
    mPlayBtnTexture.render(playBtnHitbox.x, playBtnHitbox.y);
    //SDL_RenderFillRect(mRenderer, &playBtnHitbox);
    SDL_RenderPresent(mRenderer);
}

bool MenuScene::isDone(){
    return mDone;
}

void MenuScene::handleEvent(const SDL_Event &e){
    if(e.type == SDL_MOUSEBUTTONDOWN){
        if(e.button.button == SDL_BUTTON_LEFT){
            SDL_Point mouse = {e.button.x, e.button.y};
            if(SDL_PointInRect(&mouse, &playBtnHitbox)){
                printf("Play button clicked\n");
                mManager.change(std::make_unique<GameScene>(mRenderer, mWindow, mManager));
            }
        }
    }
}

