#pragma once
#include <SDL2/SDL.h>

class Scene{
public:
    virtual ~Scene() = default;
    virtual void handleEvent(const SDL_Event& e) = 0;
    virtual void update(float deltaTime) = 0;
    virtual void render() = 0;
    virtual bool isDone() = 0;
};
