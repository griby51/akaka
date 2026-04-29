#include "Game.hpp"
#include "SceneManager.hpp"
#include "MenuScene.hpp"
#include "GameScene.hpp"
#include "LTimer.hpp"

#include <SDL2/SDL.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_joystick.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_video.h>
#include <map>


int main(int argc, char* args[]){
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_AUDIO);
    IMG_Init(IMG_INIT_PNG);
    Mix_OpenAudio(48000, MIX_DEFAULT_FORMAT, 2, 2048);
    TTF_Init();

    SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");

    std::map<int, SDL_Joystick*> openJoysticks;

    SDL_Window* window = SDL_CreateWindow(
            "Encore un jeu random",
            SDL_WINDOWPOS_UNDEFINED,
            SDL_WINDOWPOS_UNDEFINED,
            1024,
            576,
            SDL_WINDOW_SHOWN | SDL_WINDOW_FULLSCREEN_DESKTOP
    );

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    SDL_RenderSetLogicalSize(renderer, 1024, 576);

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
            if(e.type == SDL_JOYDEVICEADDED){
                int deviceIndex = e.jdevice.which;
                SDL_Joystick* joy = SDL_JoystickOpen(deviceIndex);
                if(joy){
                    int instanceId = SDL_JoystickInstanceID(joy);
                    openJoysticks[instanceId] = joy;
                    printf("Controller plugged : %d", instanceId);
                }
            }

            if(e.type == SDL_JOYDEVICEREMOVED){
                int instanceId = e.jdevice.which;
                if(openJoysticks.count(instanceId)){
                    SDL_JoystickClose(openJoysticks[instanceId]);
                    openJoysticks.erase(instanceId);
                    printf("Controller unplugged : %d\n", instanceId);
                }
            }

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

    for (auto& [id, joy] : openJoysticks) SDL_JoystickClose(joy);

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();

    return 0;
}
