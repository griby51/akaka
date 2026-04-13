#include "MenuScene.hpp"
#include "GameScene.hpp"
#include <SDL2/SDL_mouse.h>
#include <filesystem>

MenuScene::MenuScene(SDL_Renderer* renderer, SDL_Window* window ,SceneManager& manager):
    mManager(manager)
{
    mRenderer = renderer;
    mWindow = window;
    SDL_GetRendererOutputSize(mRenderer ,&mScreenWidth, &mScreenHeight);
    playBtnHitbox = {mScreenWidth / 2 - 50, mScreenHeight / 2 - 10, 100, 20}; 
    mPlayBtnTexture.setRenderer(mRenderer);
    mPlayBtnTexture.loadFromeFile("assets/buttons/playBtn.png");


    for(const auto& entry : std::filesystem::directory_iterator("assets/hats/")){
        if(entry.path().extension() == ".png"){
            LTexture tex;
            tex.setRenderer(mRenderer);
            tex.loadFromeFile(entry.path().string().c_str());
            printf("%s\n", entry.path().string().c_str());
            hats.push_back(std::move(tex));
        }
    }

    for(const auto& entry : std::filesystem::directory_iterator("assets/skins/")){
        if(entry.path().extension() == ".png"){
            LTexture tex;
            tex.setRenderer(mRenderer);
            tex.loadFromeFile(entry.path().string().c_str());
            skins.push_back(std::move(tex));
        }
    }
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
                mManager.change(std::make_unique<GameScene>(mRenderer, mWindow, mManager, mSlots, mJoinedCount));
            }
        }
    }
    if(e.type == SDL_KEYDOWN){
        SDL_Scancode key = e.key.keysym.scancode;
        for (int i = 0; i < 3; i++){
            if(presets[i].left == key ||
                presets[i].right == key ||
                presets[i].thrust == key ||
                presets[i].missile == key
              )
            {
                bool taken = false;
                for (int j = 0; j < mJoinedCount; j++){
                    if(i == mSlots[j].presetIndex){
                        taken = true;
                        break;
                    }
                }

                if(!taken){
                    mSlots[mJoinedCount].presetIndex = i;
                    mSlots[mJoinedCount].hat = &hats[0];
                    mSlots[mJoinedCount].skin = &skins[0];
                    mJoinedCount++;
                }

                printf("Preset : %i, taken: %d, playerJoined : %i\n", i, taken, mJoinedCount);
            }
        }
    }
}

