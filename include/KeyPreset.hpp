#pragma once
#include <SDL2/SDL.h>

struct KeyPreset{
    SDL_Scancode left;
    SDL_Scancode right;
    SDL_Scancode thrust;
    SDL_Scancode missile;
};

inline KeyPreset presets[3] = {                                                                       
    {SDL_SCANCODE_A, SDL_SCANCODE_D, SDL_SCANCODE_S, SDL_SCANCODE_W},                          
    {SDL_SCANCODE_J, SDL_SCANCODE_L, SDL_SCANCODE_K, SDL_SCANCODE_I},                          
    {SDL_SCANCODE_KP_4, SDL_SCANCODE_KP_6, SDL_SCANCODE_KP_5, SDL_SCANCODE_KP_8}               
 };
