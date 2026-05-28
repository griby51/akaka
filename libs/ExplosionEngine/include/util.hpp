#pragma once
#include <SDL2/SDL.h>
#include <cmath>

namespace explode::util{
    inline void fillCircle(SDL_Renderer* r, int cx, int cy, int radius){
        for(int dy = -radius; dy <= radius; dy++){
            int dx = (int)std::sqrt((float)(radius* radius - dy * dy));
            SDL_RenderDrawLine(r, cx - dx, cy + dy, cx + dx, cy + dy);
        }
    }

    inline SDL_Color lerpColor(SDL_Color a, SDL_Color b, float t){
        if(t < 0.f) t = 0.f;
        if(t > 1.f) t = 1.f;
        return {
            (Uint8)(a.r + (b.r - a.r) * t),
            (Uint8)(a.g + (b.g - a.g) * t),
            (Uint8)(a.b + (b.b - a.b) * t),
            (Uint8)(a.a + (b.a - a.a) * t)
        };
    }
    
    inline SDL_Color gradient3(SDL_Color c0, SDL_Color c1, SDL_Color c2, float t){
        if (t < 0.5f) return lerpColor(c0, c1, t * 2.f);
        return lerpColor(c1, c2, (t - 0.5f) * 2.f);
    }

    inline static float rF(float lo, float hi){
        return lo + (hi - lo) * (std::rand() / (float)RAND_MAX);
    }

    inline static float rAngle(){
        return rF(0.f, 2.f * 3.14159f);
    }
}
