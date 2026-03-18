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

        void load(GameConfig& config);
        SDL_Color getCurrentColor(int currentTime);
        int getMaxTime();

    private:
        void setColorFrameList();

        std::vector<ColorKeyframe> keyframes;
        std::vector<SDL_Color> colorFrameList;
};

class Particle{
    public:
        void init(ParticleConfig* particleConfig);
        void setPos(int posX, int posY);
        void reset();
        void update(float deltaTime);
        int getX();
        int getY();
        void render(SDL_Renderer* renderer);
        bool isAlive = false;
    private:
        SDL_Rect particleRect;
        int vx = 0;
        int vy = 0;
        float lifeTime = 0;
        float maxLifeTime;
        int intCurrentTime = 0;
        ParticleConfig* config;
};
