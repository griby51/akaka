#include "Utils.hpp"
#include <SDL2/SDL_render.h>

namespace util{
    bool collide(SDL_Rect& a, SDL_Rect& b){
        SDL_Rect intersection;
        return SDL_IntersectRect(&a, &b, &intersection) == SDL_TRUE;
    }

    bool isWithinDistance(SDL_Rect& a, SDL_Rect& b, float threshold){
        return (distSq(a, b)) < (threshold*threshold);
    }

    float distSq(SDL_Rect& a, SDL_Rect& b){
        float cx1 = a.x + a.w / 2.0f;
        float cy1 = a.y + a.h / 2.0f;

        float cx2 = b.x + b.w / 2.0f;
        float cy2 = b.y + b.h / 2.0f;

        float dx = cx2 - cx1;
        float dy = cy2 - cy1;

        return (dx*dx + dy*dy);
    }

    SDL_Rect* theNearest(SDL_Rect& tested, std::vector<SDL_Rect*> targets){
        SDL_Rect* nearest = NULL;
        float distanceSquare = 1000000000000.0f;
        for(SDL_Rect* target : targets){
            float dx = target->x + target->w / 2.0f - tested.x - tested.w / 2.0f;
            float dy = target->y + target->h / 2.0f - tested.y - tested.h / 2.0f;
            float distance = dx*dx + dy*dy;
            if(distance < distanceSquare){
                distanceSquare = distance;
                nearest = target;
            }
        }

        return nearest;
    }

    SDL_Point spawnOffScreen(int screenWidth, int screenHeight, int margin){
        int side = rand() % 4;

        SDL_Point p;

        switch(side){
            case 0: // haut
                p.x = rand() % (screenWidth + 2*margin) - margin;
                p.y = -margin;
                break;
            case 1: // bas
                p.x = rand() % (screenWidth + 2*margin) - margin;
                p.y = screenHeight + margin;
                break;
            case 2: // gauche
                p.x = -margin;
                p.y = rand() % (screenHeight + 2*margin) - margin;
                break;
            case 3: // droite
                p.x = screenWidth + margin;
                p.y = rand() % (screenHeight + 2*margin) - margin;
                break;
        }

        return p;
    }

    void drawRoundedRect(SDL_Renderer* renderer, SDL_Rect rect, int radius, SDL_Color color){
        SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
        SDL_RenderFillRect(renderer, &rect);
    }

    void drawProgressPie(SDL_Renderer* renderer, int centerX, int centerY, int radius, float progress){
        for(float angle = 0; angle < 360; angle++){
            float rad = angle * M_PI / 180.f;
            int x = centerX + radius * std::cos(rad);
            int y = centerY + radius * std::sin(rad);
            SDL_RenderDrawPoint(renderer, x, y);
        }

        if(progress > 0.f){
            if(progress >= 1.f){
                SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
            }else{
                SDL_SetRenderDrawColor(renderer, 255, 140, 0, 255);
            }

            float maxAngle = 360.f * progress;

            for (float angle = -90.f; angle < -90.f + maxAngle; angle+=0.5f){
                float rad = angle * M_PI / 180.f;
                int xEdge = centerX + radius * std::cos(rad);
                int yEdge = centerY + radius * std::sin(rad);

                SDL_RenderDrawLine(renderer, centerX, centerY, xEdge, yEdge);
            }
        }
    }
}
