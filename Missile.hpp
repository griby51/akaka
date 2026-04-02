#include "Particle.hpp"
#include <SDL2/SDL.h>
#include "Config.hpp"
#include "LTimer.hpp"
#include <math.h>

class Missile{
public:
    void init(ParticleConfig* particleConfig, GameConfig& config);
    void setPos(int posX, int posY);
    void setTarget(SDL_Rect* _target);
    void update(float deltaTime);
    void reset();
    void renderParticles(SDL_Renderer* renderer);
    double getAngleInDegree();
    int getX();
    int getY();
    bool isAlive = false;
private:
    float rotation;
    float x, y;
    float vx, vy;
    SDL_Rect* target;
    float precision = 3.0f;
    double angle = 0.0f;
    float velocity = 700.0f;
    float currentVelocity = velocity;
    static const int PARTICLE_NUMBER = 500;
    int particleSpawnTicks = 5;
    int currentParticle = 0;
    LTimer particleTimer;
    ThrustParticle particles[PARTICLE_NUMBER];
    SDL_Point particleOffset;
};
