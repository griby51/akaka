#include "Animation.hpp"
#include "LTexture.hpp"
#include "TextureManager.hpp"
#include <SDL2/SDL_render.h>
#include <cmath>

AnimationPlayer::AnimationPlayer(const Animation* animation)
    : animation(animation){}


int AnimationPlayer::getCurrentFrame() const{
    if(animation == nullptr) return 0;
    int frame = static_cast<int>(elapsed / animation->frameDuration);
    if (frame >= animation->frameCount){
        return animation->frameCount - 1;
    }else return frame;
}


void AnimationPlayer::update(float deltaTime){
    if(finished || animation == nullptr) return;
    elapsed += deltaTime;
    float total = animation->frameCount * animation->frameDuration;
    if(elapsed >= total){
        if(animation->loop) elapsed = std::fmod(elapsed, total);
        else finished = true;
    }
}

void AnimationPlayer::render(float x, float y, float scale, double angle) const{
    if(animation == nullptr || finished) return;
    LTexture* spritesheet = TextureManager::getInstance().getTexture(animation->textureId);
    
    if(spritesheet == nullptr) return;
    int frame = getCurrentFrame();
    int column = frame % animation->columns;
    int line = frame / animation->columns;

    SDL_Rect clip;
    clip.x = column * animation->frameW;
    clip.y = line * animation->frameH;
    clip.w = animation->frameW;
    clip.h = animation->frameH;

    int w = static_cast<int>(animation->frameW * scale);
    int h = static_cast<int>(animation->frameH * scale);

    spritesheet->render(x, y, &clip, angle, NULL, SDL_FLIP_NONE, w, h);
}

bool AnimationPlayer::isFinished() const{return finished; }
