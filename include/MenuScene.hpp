#pragma once
#include "Scene.hpp"
#include "SceneManager.hpp"
#include "LTexture.hpp"
#include "PlayerSlot.hpp"

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <string>
#include <vector>

class MenuScene : public Scene{
public:
    MenuScene(SDL_Renderer* renderer, SDL_Window* window, SceneManager& manager);
    ~MenuScene();

    void handleEvent(const SDL_Event &e) override;
    void update(float deltaTime) override;
    void render() override;
    bool isDone() override;

private:
    SDL_Renderer* mRenderer = nullptr;
    SceneManager& mManager;
    SDL_Window* mWindow;
    TTF_Font* mFont = nullptr;
    bool mDone = false;
    SDL_Rect playBtnHitbox;
    int mScreenWidth, mScreenHeight;
    LTexture mPlayBtnTexture;

    PlayerSlot mSlots[4];
    int mJoinedCount = 0;

    std::vector<LTexture> skins;
    std::vector<LTexture*> hats;
};
