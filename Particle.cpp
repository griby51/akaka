#include "Particle.hpp"

Particle::Particle(int x, int y, int w, int h, int minSpeed, int maxSpeed, int minAngle, int maxAngle, float lifeTime, SDL_Color colorStart, SDL_Color colorEnd, SDL_Renderer* _renderer)
{

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(minSpeed, maxSpeed);
    speed = dis(gen);

    std::uniform_int_distribution<> dis2(minAngle, maxAngle);
    direction = dis2(gen);

    rect.x = x;
    rect.y = y;
    rect.w = w;
    rect.h = h;

    velocityX = speed * cos(direction * M_PI / 180);
    velocityY = speed * sin(direction * M_PI / 180);
    life = lifeTime;
    lifeMax = lifeTime;

    endColor = colorEnd;
    startColor = colorStart;

    color = colorStart;

    renderer = _renderer;
}
void Particle::update(float deltaTime){
    rect.x += velocityX;
    rect.y += velocityY;
    life -= deltaTime;

    float ratio = life / lifeMax;

    Uint8 r = static_cast<Uint8>(startColor.r + (endColor.r - startColor.r) * ratio);
    Uint8 g = static_cast<Uint8>(startColor.g + (endColor.g - startColor.g) * ratio);
    Uint8 b = static_cast<Uint8>(startColor.b + (endColor.b - startColor.b) * ratio);
    Uint8 a = static_cast<Uint8>(startColor.a + (endColor.a - startColor.a) * ratio);


    color = {r, g, b, a};
}

bool Particle::isAlive(){
    return life > 0;
}

void Particle::render(){
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_RenderFillRect(renderer, &rect);
}