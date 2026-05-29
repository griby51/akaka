#include "Utils.hpp"

namespace util{
    bool collide(SDL_Rect& a, SDL_Rect& b){
        SDL_Rect intersection;
        return SDL_IntersectRect(&a, &b, &intersection) == SDL_TRUE;
    }

    bool isWithinDistance(SDL_Rect& a, SDL_Rect& b, float threshold){
        float cx1 = a.x + a.w / 2.0f;
        float cy1 = a.y + a.h / 2.0f;

        float cx2 = b.x + b.w / 2.0f;
        float cy2 = b.y + b.h / 2.0f;

        float dx = cx2 - cx1;
        float dy = cy2 - cy1;

        return(dx*dx + dy*dy) < (threshold*threshold);
    }
}
