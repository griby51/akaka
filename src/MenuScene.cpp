#include "MenuScene.hpp"
#include "GameScene.hpp"
#include "KeyPreset.hpp"
#include "LTexture.hpp"
#include "PlayerSlot.hpp"
#include "TextureManager.hpp"
#include "Utils.hpp"

#include <SDL2/SDL_events.h>
#include <SDL2/SDL_joystick.h>
#include <SDL2/SDL_keycode.h>
#include <SDL2/SDL_mouse.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_surface.h>
#include <SDL2/SDL_ttf.h>
#include <cstddef>

MenuScene::MenuScene(SDL_Renderer* renderer, SDL_Window* window ,SceneManager& manager):
    mManager(manager)
{
    mRenderer = renderer;
    mWindow = window;
    mScreenWidth = 1024;
    mScreenHeight = 576;
    playBtnHitbox = {mScreenWidth / 2 - 50, mScreenHeight / 2 - 10, 100, 20}; 

    mFont = TTF_OpenFont("assets/KiwiSoda.ttf", 30);

    TextureManager::getInstance().loadTexture("playBtn", "assets/buttons/playBtn.png");
    mHatIds = TextureManager::getInstance().loadDirectory("assets/hats/", "hat_");
    mSkinIds = TextureManager::getInstance().loadDirectory("assets/skins/", "skin_");
}

void MenuScene::update(float deltaTime){
}

MenuScene::~MenuScene(){
    if(mFont) TTF_CloseFont(mFont);

    TextureManager::getInstance().clean();
}

void MenuScene::render(){
    SDL_SetRenderDrawColor(mRenderer, 23, 0, 44, 255);
    SDL_RenderClear(mRenderer);

    int colWidth = mScreenWidth / 4;

    for(int i = 0; i < 4; i++){
        int colX = i * (mScreenWidth / 4);

        if(i < mJoinedCount){
            drawPanel(colX + 10, 50, colWidth - 20, 500, mSlots[i].ready);

            drawPlayer(colX, colWidth, mSlots[i].skinId, mSlots[i].hatId);

            SDL_Color white = {255, 255, 255, 255};
            SDL_Color green = {0, 255, 255, 255};

            renderText(mSlots[i].hatId, colX + 20, 350, (mSlots[i].menuCursorY == 0) ? green : white);
            renderText(mSlots[i].skinId, colX + 20, 400, (mSlots[i].menuCursorY == 1) ? green : white);
            renderText("Come later", colX + 20, 450, (mSlots[i].menuCursorY == 2) ? green : white);
            renderText("Ready ?", colX + 20, 500, (mSlots[i].menuCursorY == 3) ? green : white);

            if(mSlots[i].ready){
                renderText("[ READY ]", colX + 60, 550, SDL_Color{255, 255, 0, 255});
            }
        }else{
            drawEmptySlot(colX, colWidth);
        }
    }
    
    SDL_RenderPresent(mRenderer);
}

bool MenuScene::isDone(){
    return mDone;
}

void MenuScene::handleEvent(const SDL_Event &e){
    if(e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE){
        mManager.pop();
        mDone = true;
        return;
    }

    bool hasInput = false;
    int inputPresetId = -1;
    int inputJoyId = -1;

    bool navUp = false, navDown = false, navLeft = false, navRight = false;
    bool actReady = false, actCancel = false;

    if(e.type == SDL_KEYDOWN){
        SDL_Scancode key = e.key.keysym.scancode;
        for(int i = 0; i < 3; i++){
            if(key == presets[i].left){
                inputPresetId = i;
                navLeft = true;
                hasInput = true;
                break;
            }
            if(key == presets[i].right){
                inputPresetId = i;
                navRight = true;
                hasInput = true;
                break;
            }
            if(key == presets[i].thrust){
                inputPresetId = i;
                navDown = true;
                hasInput = true;
                break;
            }
            if(key == presets[i].missile){
                inputPresetId = i;
                navUp = true;
                hasInput = true;
                break;
            }
        }
    }else if(e.type == SDL_JOYHATMOTION){
        inputJoyId = e.jhat.which;
        if(e.jhat.value != SDL_HAT_CENTERED){
            hasInput = true;
            if(e.jhat.value & SDL_HAT_UP) navUp = true;
            if(e.jhat.value & SDL_HAT_DOWN) navDown = true;
            if(e.jhat.value & SDL_HAT_LEFT) navLeft = true;
            if(e.jhat.value & SDL_HAT_RIGHT) navRight = true;
        }
    }else if(e.type == SDL_JOYAXISMOTION){
        inputJoyId = e.jaxis.which;
        hasInput = true;
    }else if(e.type == SDL_JOYBUTTONDOWN){
        inputJoyId = e.jbutton.which;
        hasInput = true;
    }

    if(!hasInput) return;

    int pIndex = -1;
    for(int i = 0; i < mJoinedCount; i++){
        if(inputJoyId != -1 && mSlots[i].joystickId == inputJoyId){
            pIndex = i;
            break;
        }
        if(inputPresetId != -1 && mSlots[i].presetIndex == inputPresetId){
            pIndex = i;
            break;
        }
    }

    if(pIndex == -1 && mJoinedCount < 4){
        if(e.type == SDL_JOYAXISMOTION && abs(e.jaxis.value) < 8000) return;

        pIndex = mJoinedCount;
        mSlots[pIndex].presetIndex = inputPresetId;
        mSlots[pIndex].joystickId = inputJoyId;
        mSlots[pIndex].menuCursorY = 0;
        mSlots[pIndex].ready = false;
        mSlots[pIndex].lastAxisX = 0;
        mSlots[pIndex].lastAxisY = 0;
        mSlots[pIndex].skinId = mSkinIds[mSlots[pIndex].skinIndex];
        mSlots[pIndex].hatId = mHatIds[mSlots[pIndex].hatIndex];
        mJoinedCount++;
    }

    if(pIndex != -1){
        PlayerSlot& slot = mSlots[pIndex];

        if(e.type == SDL_JOYAXISMOTION){
            const int DEADZONE = 8000;
            int currentAxisVal = 0;

            if(e.jaxis.value < -DEADZONE) currentAxisVal = -1;
            else if(e.jaxis.value > DEADZONE) currentAxisVal = 1;

            if(e.jaxis.axis == 0){
                if(currentAxisVal == -1 && slot.lastAxisX != -1) navLeft = true;
                if(currentAxisVal == 1 && slot.lastAxisX != 1) navRight = true;
                slot.lastAxisX = currentAxisVal;
            }else if(e.jaxis.axis == 1){
                if(currentAxisVal == -1 && slot.lastAxisY != -1) navUp = true;
                if(currentAxisVal == 1 && slot.lastAxisY != 1) navDown = true;
                slot.lastAxisY = currentAxisVal;
            }
        }

        if(slot.ready){
            if(actCancel) slot.ready = false;
        }else{
            if(navUp) slot.menuCursorY = std::max(0, slot.menuCursorY - 1);
            if(navDown) slot.menuCursorY = std::min(3, slot.menuCursorY + 1);

            if(navLeft || navRight){
                int dir = navRight ? 1 : -1;

                if(slot.menuCursorY == 0 && mHatIds.size() > 0){
                    slot.hatIndex = (slot.hatIndex + dir + mHatIds.size()) % mHatIds.size();
                    slot.hatId = mHatIds[slot.hatIndex];
                }

                if(slot.menuCursorY == 1 && mSkinIds.size() > 0){
                    slot.skinIndex = (slot.skinIndex + dir + mSkinIds.size()) % mSkinIds.size();
                    slot.skinId = mSkinIds[slot.skinIndex];
                }

            }
        }
        if((actReady || navLeft || navRight) && slot.menuCursorY == 3){
            slot.ready = !slot.ready;
            bool allReady = true;
            for(int i = 0; i < mJoinedCount; i++){
                allReady = mSlots[i].ready;
                if(!allReady) break;
            }

            if(allReady) startGame();
        }
    }
}

void MenuScene::startGame(){
    if(mJoinedCount <= 0){
        printf("No player");
    }else{
        mManager.push(std::make_unique<GameScene>(mRenderer, mWindow, mManager, mSlots, mJoinedCount));
    }
}

void MenuScene::drawPanel(int x, int y, int w, int h, bool isReady){
        SDL_Rect rect {x, y, w, h};

        SDL_Color bgColor = isReady ? SDL_Color{8, 45, 92, 255} : SDL_Color{0, 86, 135, 255};

        util::drawRoundedRect(mRenderer, rect, 15, bgColor);

        SDL_SetRenderDrawColor(mRenderer, 68, 220, 197, 255);
        SDL_RenderDrawRect(mRenderer, &rect);
}

void MenuScene::drawPlayer(int colX, int colWidth, std::string skinId, std::string hatId){
    int scale = 5;
    int size = 32 * scale;

    int centerX = colX + (colWidth / 2) - (size / 2);
    int baseY = 150;

    TextureManager::getInstance().getTexture(skinId)->render(centerX, baseY, NULL, 0.0, NULL, SDL_FLIP_NONE, size, size);
    TextureManager::getInstance().getTexture(hatId)->render(centerX, baseY, NULL, 0.0, NULL, SDL_FLIP_NONE, size, size);
}

void MenuScene::renderText(std::string text, int x, int y, SDL_Color color){
    SDL_Surface* surf = TTF_RenderText_Blended(mFont, text.c_str(), color);
    SDL_Texture* tex = SDL_CreateTextureFromSurface(mRenderer, surf);

    SDL_Rect dest = {x, y, surf->w, surf->h};
    SDL_RenderCopy(mRenderer, tex, NULL, &dest);

    SDL_FreeSurface(surf);
    SDL_DestroyTexture(tex);
}

void MenuScene::drawEmptySlot(int x, int colWidth){
    SDL_Rect rect = {x + 10, 50, colWidth - 20, 500};
    SDL_SetRenderDrawColor(mRenderer, 30, 30, 30, 150);
    SDL_RenderFillRect(mRenderer, &rect);

    renderText("Press a touch", x + (colWidth/2) - 70, 250, SDL_Color{255, 255, 255, 255});
}
