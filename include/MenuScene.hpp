#pragma once
#include "Scene.hpp"
#include "SceneManager.hpp"
#include "LTexture.hpp"
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <string>


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
};
