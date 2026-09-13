#pragma once

#include "TextureManager.hpp"

struct Animation{
    std::string textureId;
    int frameW, frameH;
    int columns;
    int frameCount;
    float frameDuration;
    bool loop = false;
};
