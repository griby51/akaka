#include "Particle.hpp"
#include <SDL2/SDL.h>
#include "LTexture.hpp"

class Missile{
public:
    void init(LTexture* _texture, SDL_Rect* missileRect);
    void setPos(int posX, int posY);
    void setTarget(SDL_Rect* _target);
    void update(float deltaTime);
    void render(SDL_Renderer* renderer);
    void reset();

    bool isAlive = false;
private:
    float rotation;


}
