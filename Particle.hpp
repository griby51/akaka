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
        float growRate;
        float friction;
        float riseForce;
        int vxSpread;
        int vyMin;
        int vyMax;
        int sizeMin;
        int sizeMax;

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
        void setGlobalSpeed(float _globalSpeed = 1);
    private:
        SDL_Rect particleRect;
        int vx = 0;
        int vy = 0;
        float size = 10;
        float growRate;
        float friction;
        float riseForce;
        int vxSpread;
        int vyMin, vyMax;
        float lifeTime = 0;
        float maxLifeTime;
        int intCurrentTime = 0;
        ParticleConfig* config;
        float globalSpeed;
};
