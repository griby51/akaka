#include "Game.hpp"
#include "SceneManager.hpp"
#include "MenuScene.hpp"
#include "GameScene.hpp"
#include "LTimer.hpp"

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_video.h>

int main(int argc, char* args[]){
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK);
    IMG_Init(IMG_INIT_PNG);
    TTF_Init();

    SDL_Window* window = SDL_CreateWindow(
            "Encore un jeu random",
            SDL_WINDOWPOS_UNDEFINED,
            SDL_WINDOWPOS_UNDEFINED,
            800,
            600,
            SDL_WINDOW_SHOWN
    );

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    SceneManager manager;
    manager.push(std::make_unique<MenuScene>(renderer, window, manager));

    LTimer deltaTimer;
    deltaTimer.start();

    SDL_Event e;
    while (!manager.isEmpty()){
        float deltaTime = deltaTimer.getTicks() / 1000.0f;
        if(deltaTime > 0.05f){
            deltaTime = 0.05f;
        }
        deltaTimer.start();

        while(SDL_PollEvent(&e)){
            if (e.type == SDL_QUIT){
                while(!manager.isEmpty()){
                    manager.pop();
                }
            }
            if(manager.current()){
                manager.current()->handleEvent(e);
            }
        }

        if(!manager.current()){
            break;
        }

        manager.current()->update(deltaTime);
        manager.current()->render();
        SDL_Delay(16);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();

    return 0;
}
