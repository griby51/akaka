#include "LTexture.hpp"

LTexture::LTexture(){
    mTexture = NULL;
    mWidth = 0;
    mHeight = 0;
}

LTexture::~LTexture(){
    free();
}

void LTexture::setRenderer(SDL_Renderer* renderer){
    gRenderer = renderer;
}

bool LTexture::loadFromeFile(std::string path){
    free();

    SDL_Texture* newTexture = NULL;

    SDL_Surface* loadedSurface = IMG_Load(path.c_str());
    if(loadedSurface == NULL){
        printf("Unable to load image %s! SDL Error : %s\n", path.c_str(), SDL_GetError());
    }else{
        SDL_SetColorKey(loadedSurface, SDL_TRUE, SDL_MapRGB(loadedSurface->format, 0, 0xFF, 0xFF));

        newTexture = SDL_CreateTextureFromSurface(gRenderer, loadedSurface);
        if (newTexture == NULL) printf("Unable to create texture from %s ! SDL Error : %s\n", path.c_str(), SDL_GetError());
        else{
            mWidth = loadedSurface->w;
            mHeight = loadedSurface->h;
        }

        SDL_FreeSurface(loadedSurface);
    }

    mTexture = newTexture;
    return mTexture != NULL;
}

void LTexture::free()
{
    if(mTexture != NULL){
        SDL_DestroyTexture(mTexture);
        mTexture = NULL;
        mWidth = 0;
        mHeight = 0;
    }
}

void LTexture::render(int x, int y, SDL_Rect* clip, double angle, SDL_Point* center, SDL_RendererFlip flip){
    SDL_Rect renderQuad = {x, y, mWidth, mHeight};

    if(clip != NULL){
        renderQuad.w = clip->w;
        renderQuad.h = clip->h;
    }

    SDL_RenderCopyEx(gRenderer, mTexture, clip, &renderQuad, angle, center, flip);
}

void LTexture::setColor(Uint8 red, Uint8 green, Uint8 blue){
    if(SDL_SetTextureColorMod(mTexture, red, green, blue) < 0) printf("Error while changing color : %s\n", SDL_GetError());
}

void LTexture::setBlendMode(SDL_BlendMode blending){
    SDL_SetTextureBlendMode(mTexture, blending);
}

void LTexture::setAlpha(Uint8 alpha){
    SDL_SetTextureAlphaMod(mTexture, alpha);
}

int LTexture::getWidth(){ return mWidth; }

int LTexture::getHeight(){ return mHeight; }

bool LTexture::loadFromRenderedText(std::string textureText, SDL_Color textColor, TTF_Font* gFont){
    free();

    SDL_Surface* textSurface = TTF_RenderText_Solid(gFont, textureText.c_str(), textColor);

    if(textSurface == NULL){
        printf("Unable to render text surface. SDL_ttf error : %s\n", TTF_GetError);
    }else{
        mTexture = SDL_CreateTextureFromSurface(gRenderer, textSurface);
        if(mTexture == NULL){
            printf("Unable to create texture from renderered text! SDL Error : %s", SDL_GetError());
        }else{
            mWidth = textSurface->w;
            mHeight = textSurface->h;
        }

        SDL_FreeSurface(textSurface);
    }

    return mTexture != NULL;
}
