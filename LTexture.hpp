#pragma once

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <string>
#include <stdio.h>
#include <SDL2/SDL_ttf.h>

class LTexture
{
    public:
        LTexture();
        ~LTexture();

        void setRenderer(SDL_Renderer* renderer);

        bool loadFromeFile(std::string path);

        bool loadFromRenderedText(std::string textureText, SDL_Color textColor, TTF_Font* gFont);

        void free();

        void setColor(Uint8 red, Uint8 green, Uint8 blue);

        void setBlendMode(SDL_BlendMode blending);
        
        void setAlpha(Uint8);

        void render(int x, int y, SDL_Rect* clip = NULL, double angle = 0.0, SDL_Point* center = NULL, SDL_RendererFlip flip = SDL_FLIP_NONE);

        int getWidth();
        int getHeight();
    
    private: 
        SDL_Texture* mTexture;

        SDL_Renderer* gRenderer;
        int mWidth;
        int mHeight;
};