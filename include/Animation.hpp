#pragma once

#include <string>

struct Animation{
    std::string textureId;
    int frameW = 64, frameH = 64;
    int columns = 5;
    int frameCount = 5;
    float frameDuration = 10;
    bool loop = false;
};

class AnimationPlayer{
public:
    AnimationPlayer(const Animation* animation);

    void update(float deltaTime);
    void render(float x, float y, float scale = 1.f, double angle = 0.0) const;

    bool isFinished() const;
    int getCurrentFrame() const;

private:
    const Animation* animation = nullptr;
    float elapsed = 0.f;
    bool finished = false;
};
