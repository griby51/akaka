#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include <string>
#include <sstream>
#include "LTexture.hpp"
#include "LTimer.hpp"
#include "Projectile.hpp"
#include "Dot.hpp"
#include "Player.hpp"
#include "CollisionSystem.hpp"

const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;

const int SCREEN_FPS = 60;
const int SCREEN_TICKS_PER_FRAME = 1000/SCREEN_FPS;

const int GLOBAL_SPEED = 5;

bool init();
bool loadMedia();
void close();

SDL_Window* gWindow = NULL;
SDL_Renderer* gRenderer = NULL;

SDL_Rect gSpritesClips[2];
SDL_Rect gProjectileRect;

Player player;

LTexture gSpriteSheetTexture;
LTexture gMortar;
LTexture gProjectile;
LTexture gDotTexture;
LTexture gBGTexture;
LTexture gScoreTexture;

TTF_Font* gScoreFont = NULL;

SDL_Color RED = {255, 0, 0, 255};
SDL_Color WHITE = {255, 255, 255, 255};

const int playerMaxVelX = 10;
const int playerMaxVelY = 10;

int playerVelX = 0;
int playerVelY = 0;

const int PROJECTILE_ARRAY_SIZE = 20;
Projectile projectiles[PROJECTILE_ARRAY_SIZE];

int playerX = SCREEN_WIDTH / 2;
int playerY = SCREEN_HEIGHT / 2;

bool init(){
    bool success = true;

    if(SDL_Init(SDL_INIT_VIDEO) < 0){
        printf("SDL could not initialize : %s\n", SDL_GetError());
        success = false;
    }
    else{
        gWindow = SDL_CreateWindow("Encore unjeu random", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
        if (gWindow == NULL){
            printf("Error initializing window %s\n", SDL_GetError());
            success = false;
        }else{
            gRenderer = SDL_CreateRenderer(gWindow, -1, SDL_RENDERER_ACCELERATED);
            if(gRenderer == NULL){
                printf("Renderer could not be created! SDL Error: %s\n", SDL_GetError());
                success = false;
            }else{
                int imgFlags = IMG_INIT_PNG;
                if(!(IMG_Init(imgFlags) & imgFlags)){
                    printf("SDL_image could not be initialized! SDL_image Error %s\n", IMG_GetError());
                    success = false;
                }

                if(TTF_Init() == -1){
                    printf("SDL_ttf could not initialize! SDL_ttf Error %s\n", TTF_GetError());
                    success = false;
                }

            }
        }
    }

    return success;
}

bool loadMedia(){
    bool success = true;

    gScoreFont = TTF_OpenFont("Hypermonosaturation-zrMo0.ttf", 28);
    if(gScoreFont == NULL){
        printf("Failed to load font ! SDL_Error : %s", TTF_GetError());
        success = false;
    }else{
        if(!gScoreTexture.loadFromRenderedText("Score : 0", WHITE, gScoreFont)){
            printf("Failed to render text texure !\n");
            success = false;
        }
    }

    if(!gDotTexture.loadFromeFile("dot.bmp")){
        printf("Failed to load dot texture\n");
        success = false;
    }

    if(!gBGTexture.loadFromeFile("abstract_seamless_bg_01.png")){
        printf("Failed to load background texture\n");
        success = false;
    }

    if(!gMortar.loadFromeFile("HeavyMortar.png")){
        printf("Failed to load Heavy Mortar texture\n");
        success = false;
    }

    if(!gProjectile.loadFromeFile("cannonbob.png")){
        printf("Failed to laod cannonbob texture");
        success = false;
    }else{
        gProjectileRect.x = 0;
        gProjectileRect.y = 0;
        gProjectileRect.w = 16;
        gProjectileRect.h = 16;
    }



    if(!gSpriteSheetTexture.loadFromeFile("player.png")){
        printf("Failed to load player spritesheet texture\n");
        success  = false;
    }else{

        gSpritesClips[0].x = 0;
        gSpritesClips[0].y = 0;
        gSpritesClips[0].w = 32;
        gSpritesClips[0].h = 32;

        gSpritesClips[1].x = 0;
        gSpritesClips[1].y = 32;
        gSpritesClips[1].w = 32;
        gSpritesClips[1].h = 32;
    }

    return success;
}

void close(){
    gSpriteSheetTexture.free();
    gMortar.free();
    gProjectile.free();
    gDotTexture.free();
    gBGTexture.free();
    gScoreTexture.free();

    SDL_DestroyRenderer(gRenderer);
    SDL_DestroyWindow(gWindow);

    gWindow = NULL;
    gRenderer = NULL;

    TTF_Quit();
    IMG_Quit();
    SDL_Quit();
}

int main(int argc, char* args[]){
    if(!init()){
        printf("Failed to init");
        return -1;
    }

    gSpriteSheetTexture.setRenderer(gRenderer);
    gMortar.setRenderer(gRenderer);
    gProjectile.setRenderer(gRenderer);
    gDotTexture.setRenderer(gRenderer);
    gBGTexture.setRenderer(gRenderer);
    gScoreTexture.setRenderer(gRenderer);

    if(!loadMedia()){
        printf("Failed to load media");
        return -1;
    }
    srand(time(0));

    bool quit = false;

    player.setScreenSize(SCREEN_WIDTH, SCREEN_HEIGHT);

    SDL_Event e;

    SDL_Color textColor = {0, 0, 0, 255};

    // LTimer fpsTimer;

    LTimer capTimer;
    LTimer projectileTimer;
    LTimer mortarTimer;
    LTimer mortarProjectileTimer;
    LTimer collisionTimer;
    LTimer scoreTimer;
    LTimer deltaTimer;

    std::stringstream timeText;
    scoreTimer.start();

    player.setPos(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2);
    player.setCollider(0, 0, 32, 32);

    int currentProj = 0;

    for (int i = 0; i <= PROJECTILE_ARRAY_SIZE; i++){
        projectiles[i].setPos(10000, 10000);
        projectiles[i].setCollider(0, 0, 16, 16);
    };

    int randomProjectilePos = rand() % SCREEN_HEIGHT;
    int countedFrames = 0;
    // fpsTimer.start();

    projectileTimer.start();
    mortarTimer.start();
    collisionTimer.start();

    int randomBaseProjectileSpawnTicks = rand() % 3000;
    int randomMortarSpawnTicks = rand() % 20000;

    randomMortarSpawnTicks = 3000;

    int mortarX = 10000;
    int mortarY = SCREEN_HEIGHT - gMortar.getHeight();

    int scrollingOffset = 0;

    Dot dot;


    while(!quit){
        while(SDL_PollEvent(&e) != 0){
            if(e.type == SDL_QUIT){
                quit = true;
            }
        }

        float deltaTime = deltaTimer.getTicks() / 1000.0;
        if(deltaTime > 0.05) deltaTime = 0.05;

        dot.move();

        scrollingOffset -= GLOBAL_SPEED;

        if (scrollingOffset < -gBGTexture.getWidth()){
            scrollingOffset = 0;
        }

        // float avgFPS = countedFrames / (fpsTimer.getTicks() / 1000.f);
        // if (avgFPS > 2000000) avgFPS = 0;
        // // printf("FPS : %g\n", avgFPS);

        if(projectileTimer.getTicks() >= randomBaseProjectileSpawnTicks){
            randomBaseProjectileSpawnTicks = rand() % 1000;
            randomProjectilePos = rand() % SCREEN_HEIGHT;
            projectileTimer.start();
            projectiles[currentProj].setPos(SCREEN_WIDTH, randomProjectilePos);
            projectiles[currentProj].setVelocity(-8, 0);

            if (currentProj++ >= PROJECTILE_ARRAY_SIZE) currentProj = 0;
        }

        if(mortarTimer.getTicks() >= randomMortarSpawnTicks){
            randomMortarSpawnTicks = rand() % 3000 + 5000;
            mortarTimer.start();
            mortarX = SCREEN_WIDTH;
        }

        const Uint8* currentKeyStates = SDL_GetKeyboardState(NULL);
        if(currentKeyStates[SDL_SCANCODE_D]){
            player.move(1);
        }
        if(currentKeyStates[SDL_SCANCODE_A]){
            player.move(-1);
        }
        if(currentKeyStates[SDL_SCANCODE_SPACE]){
            player.jetpack();
        }

        SDL_SetRenderDrawColor(gRenderer, 135, 206, 235, 0xFF);
        SDL_RenderClear(gRenderer);

        gBGTexture.render(scrollingOffset, 0);
        gBGTexture.render(scrollingOffset + gBGTexture.getWidth(), 0);

        gSpriteSheetTexture.render(player.getX(), player.getY(), &gSpritesClips[0]);
        player.drawCollider(gRenderer, &WHITE);

        bool mortarOnScreen = (mortarX > -30) && mortarX < SCREEN_WIDTH;

        if(mortarOnScreen){
            if(!mortarProjectileTimer.isStarted() || mortarProjectileTimer.isPaused()){
                mortarProjectileTimer.start();
            }else if(mortarProjectileTimer.getTicks() >= 500){
                mortarProjectileTimer.start();
                projectiles[currentProj].setPos(mortarX, mortarY);
                projectiles[currentProj].setVelocity(-2*GLOBAL_SPEED, -GLOBAL_SPEED);
                if (currentProj++ >= PROJECTILE_ARRAY_SIZE) currentProj = 0;
            }

            gMortar.render(mortarX, mortarY, NULL, 0.0, NULL, SDL_FLIP_HORIZONTAL);
        }

        mortarX -= GLOBAL_SPEED;

        for (int i = 0; i<=PROJECTILE_ARRAY_SIZE; i++){ 
            projectiles[i].update();
            bool isInScreen = (
                (projectiles[i].getX() > 0 - gProjectileRect.w) && 
                (projectiles[i].getX() < SCREEN_WIDTH) && 
                (projectiles[i].getY() > 0 - gProjectileRect.h) && 
                (projectiles[i].getY() < SCREEN_HEIGHT)
            );

            if (isInScreen){
                gProjectile.render(projectiles[i].getX(), projectiles[i].getY(), &gProjectileRect);
                projectiles[i].drawCollider(gRenderer, &RED);

                if(Collision::collide(&projectiles[i].collider, &player.collider)){
                    if (collisionTimer.getTicks() > 500){
                        collisionTimer.start();
                        player.updateScore(-100);
                        std::string scoreText = "Score : " + std::to_string(player.getScore());
                        gScoreTexture.loadFromRenderedText(scoreText, WHITE, gScoreFont);
                    }
                }
            }
        }

        player.update(deltaTime);

        if (scoreTimer.getTicks() >= 50){
            scoreTimer.start();
            player.updateScore(1);
            std::string scoreText = "Score : " + std::to_string(player.getScore());
            gScoreTexture.loadFromRenderedText(scoreText, WHITE, gScoreFont);
        }

        gScoreTexture.render((SCREEN_WIDTH - gScoreTexture.getWidth()) / 2, 50);

        SDL_RenderPresent(gRenderer);

        ++countedFrames;

        deltaTimer.start();

        int frameTicks = capTimer.getTicks();
        if(frameTicks < SCREEN_TICKS_PER_FRAME){
            SDL_Delay(SCREEN_TICKS_PER_FRAME - frameTicks);
        }
    }

    close();

    return 0;
}