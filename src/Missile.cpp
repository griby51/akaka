#include "Missile.hpp"

void Missile::init(ParticleConfig* particleConfig, GameConfig& config){
    for(int i = 0; i < PARTICLE_NUMBER; i++){
        particles[i].init(particleConfig);
    }

    precision = config.getFloat("precision", 3.0f);
    velocity = config.getFloat("velocity", 700.0f);
    angle = config.getFloat("angle", -90.0f);

    collider.h = 10;
    collider.w = 10;
    collider.x = 0;
    collider.y = 0;
}

void Missile::setPos(int posX, int posY){
    x = posX;
    y = posY;
    collider.x = posX;
    collider.y = posY;
}

void Missile::setTarget(SDL_Rect* _target){
    target = _target;
}

void Missile::update(float deltaTime){
    for(int i = 0; i < PARTICLE_NUMBER; i++){
        particles[i].update(deltaTime);
    }

    if (!isAlive) return;

    int targetX = target->x + (target->w / 2);
    int targetY = target->y + (target->h / 2);
    float xDist = targetX - x;
    float yDist = targetY - y;
    float dist = sqrt(pow(xDist, 2) + pow(yDist, 2));

    double targetedAngle = atan2(yDist, xDist) + M_PI;

    double diff = targetedAngle - angle;
    while (diff > M_PI) diff -= 2 * M_PI;
    while (diff < -M_PI) diff += 2 * M_PI;

    if (diff > 0){
        angle -= precision*deltaTime;
    }else{
        angle += precision*deltaTime;
    }

    float turnRatio = (std::abs(diff) / M_PI);
    float targetVelocity = velocity * std::max(0.0f, turnRatio);

    float lerpSpeed = 3.0f;
    currentVelocity += (targetVelocity - currentVelocity) * lerpSpeed * deltaTime;

    vx = std::cos(angle) * currentVelocity;
    vy = std::sin(angle) * currentVelocity;

    x+=vx * deltaTime;
    y+=vy * deltaTime;
    collider.x = x;
    collider.y = y;

    float centerX = x + 16;
    float centerY = y + 16;

    float backX = centerX - std::cos(angle) * 16;
    float backY = centerY - std::sin(angle) * 16;

    if(particleTimer.getTicks() >= particleSpawnTicks){
        particleTimer.start();
        particles[currentParticle].setPos(backX, backY);
        particles[currentParticle].reset();
        currentParticle = (currentParticle + 1) % PARTICLE_NUMBER;
    }
}

double Missile::getAngleInDegree(){
    return angle * (180 / M_PI);
}

int Missile::getX(){
    return x;
}

int Missile::getY(){
    return y;
}

void Missile::renderParticles(SDL_Renderer* renderer){
    for(int i = 0; i < PARTICLE_NUMBER; i++){
        particles[i].render(renderer);
    }
}

void Missile::reset(){
    angle = -90.0f;
    particleTimer.start();
    for(int i = 0; i < PARTICLE_NUMBER; i++){
        particles[i].reset();
        particles[i].setPos(10000, 10000);
    }
}

void Missile::drawCollider(SDL_Renderer* renderer, SDL_Color* color){
    SDL_SetRenderDrawColor(renderer, color->r, color->g, color->b, color->a);
    SDL_RenderDrawRect(renderer, &collider);
}
