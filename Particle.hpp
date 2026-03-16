#pragma once
#include <SDL2/SDL.h>
#include "Config.hpp"
#include <vector>

struct ColorKeyframe{
    Uint8 r, g, b, a;
    int time;
};

class ParticleConfig{
    public:
        std::vector<ColorKeyframe> keyframes;

        void load(GameConfig& config);
        SDL_Color getCurrentColor(int currentTime);

    private:
        void setColorFrameList();

        std::vector<SDL_Color> colorFrameList;
};
