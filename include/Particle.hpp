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
    bool isAlive = false;
    void setPos(int posX, int posY);
    int getX();
    int getY();
    
    virtual void render(SDL_Renderer* renderer) = 0;
    virtual void reset() = 0;
    virtual void update(float deltaTime) = 0;
protected:
    SDL_Rect particleRect;
    float size = 10;
    float fx, fy;
    float lifeTime = 0;
    float maxLifeTime;
    int intCurrentTime = 0;
};

class ThrustParticle : public Particle{
public:
    void init(ParticleConfig* config);
    void reset() override;
    void update(float deltaTime) override;
    void render(SDL_Renderer* renderer) override;

private:
    float vx = 0;
    float vy = 0;

    float growRate;
    float friction;
    float riseForce;
    float vyMin, vyMax;
    ParticleConfig* config;
};


class ExplosionParticle : public Particle{
public:
    void init(ParticleConfig* _colorsConfig);
    void reset() override;
    void update(float deltaTime) override;
    void render(SDL_Renderer* renderer) override;

private:
    float vx, vy;
    float power;
    float friction;
    float angle;
    float gravity;
    float lifeTime;

    ParticleConfig* colorsConfig;
};
