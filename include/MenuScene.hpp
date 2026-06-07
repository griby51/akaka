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

    PlayerSlot mSlots[4];
    int mJoinedCount = 0;

    std::vector<std::string> mSkinIds;
    std::vector<std::string> mHatIds;
    
    void startGame();

    void drawPanel(int x, int y, int w, int h, bool isReady);
    void drawPlayer(int colX, int colWidth, std::string skinId, std::string hatId);
    void renderText(std::string text, int x, int y, SDL_Color color);
    void drawEmptySlot(int x, int colWidth);
};
