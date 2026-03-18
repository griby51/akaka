#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include "Config.hpp"
#include <stdio.h>
#include <string>
#include <sstream>
#include "LTexture.hpp"
#include "LTimer.hpp"
#include "Projectile.hpp"
#include "Dot.hpp"
#include "Player.hpp"
#include "Particle.hpp"
#include "CollisionSystem.hpp"
#include <iostream>


GameConfig g_config("config.ini");
int SCREEN_WIDTH = g_config.getInt("SCREEN_WIDTH", 800);
int SCREEN_HEIGHT = g_config.getInt("SCREEN_HEIGHT", 600);

GameConfig thrustParticleGameConfig("playerThrustParticle.ini");
ParticleConfig thrustParticleConfig;

const int SCREEN_FPS = 60;
const int SCREEN_TICKS_PER_FRAME = 1000/SCREEN_FPS;

const int JOYSTICK_DEAD_ZONE = 8000;
const int THRUST_PARTICLE_NUMBER = 500;

const int GLOBAL_SPEED = 5;

bool init();
bool loadMedia();
void close();

SDL_Window* gWindow = NULL;
SDL_Renderer* gRenderer = NULL;

SDL_Joystick* gGameController = NULL;
SDL_Rect gSpritesClips[2];
SDL_Rect gProjectileRect;
SDL_Rect gPlayer2Clip;
SDL_Rect gCactusManRect;

const int PLAYER_NUMBER = 2;
Player player[PLAYER_NUMBER];
Particle thrustParticles[THRUST_PARTICLE_NUMBER];


LTexture gSpriteSheetTexture;
LTexture gPlayer2Texture;
LTexture gMortar;
LTexture gProjectile;
LTexture gDotTexture;
LTexture gBGTexture;
LTexture gScoreTexture;

TTF_Font* gScoreFont = NULL;

SDL_Color RED = {255, 0, 0, 255};
SDL_Color WHITE = {255, 255, 255, 255};

const int PROJECTILE_ARRAY_SIZE = 20;
Projectile projectiles[PROJECTILE_ARRAY_SIZE];

bool init(){
    bool success = true;

    
    for(int i = 0; i < PLAYER_NUMBER; i++){
        player[i].init(&g_config);
    }

    if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK) < 0){
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
        if(SDL_NumJoysticks() < 1){
            printf("No joysticks conneceted");
        }
        else{
            gGameController = SDL_JoystickOpen(0);
            if(gGameController == NULL){
                printf("Warning : Unable to open game controller ! SDL Error : %s\n", SDL_GetError());
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

    if(!gPlayer2Texture.loadFromeFile("dawntree.png")){
        printf("Failed to load player 2 texture\n");
        success = false;
    }
    else{
        gPlayer2Clip.x = 0;
        gPlayer2Clip.y = 0;
        gPlayer2Clip.w = 32;
        gPlayer2Clip.h = 32;
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
    
thrustParticleConfig.load(thrustParticleGameConfig);

    for(int i = 0; i < THRUST_PARTICLE_NUMBER; i++){
        thrustParticles[i].init(&thrustParticleConfig);
    };
    g_config.save();

    SDL_SetRenderDrawBlendMode(gRenderer, SDL_BLENDMODE_BLEND);
    gSpriteSheetTexture.setRenderer(gRenderer);
    gPlayer2Texture.setRenderer(gRenderer);
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

    int xJoystickDir = 0;
    int yJoystickDir = 0;

    for (int i = 0; i<PLAYER_NUMBER; i++){
        player[i].setScreenSize(SCREEN_WIDTH, SCREEN_HEIGHT);
    }

    SDL_Event e;

    // LTimer fpsTimer;

    LTimer capTimer;
    LTimer projectileTimer;
    LTimer mortarTimer;
    LTimer mortarProjectileTimer;
    LTimer collisionTimer;
    LTimer scoreTimer;
    LTimer deltaTimer;
    LTimer player1ParticleTimer;
    LTimer player2ParticleTimer;

    std::stringstream timeText;
    scoreTimer.start();

    player[0].setPos(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 4);
    player[1].setPos(SCREEN_WIDTH / 2, (SCREEN_HEIGHT / 4) * 3);

    for(int i = 0; i < PLAYER_NUMBER; i++){
        player[i].setCollider(0, 0, 32, 32);
    }

    int currentProj = 0;

    for (int i = 0; i < PROJECTILE_ARRAY_SIZE; i++){
        projectiles[i].setPos(10000, 10000);
        projectiles[i].setCollider(0, 0, 16, 16);
    };

    int randomProjectilePos = rand() % SCREEN_HEIGHT;
    int countedFrames = 0;

    // fpsTimer.start();

    projectileTimer.start();
    mortarTimer.start();
    collisionTimer.start();
    player1ParticleTimer.start();
    player2ParticleTimer.start();

    int randomBaseProjectileSpawnTicks = rand() % 3000;
    int randomMortarSpawnTicks = rand() % 20000;

    randomMortarSpawnTicks = 3000;

    int mortarX = 10000;
    int mortarY = SCREEN_HEIGHT - gMortar.getHeight();

    int currentThrustParticle = 0;

    int scrollingOffset = 0;

    Dot dot;


    while(!quit){
        while(SDL_PollEvent(&e) != 0){
            if(e.type == SDL_QUIT){
                quit = true;
            }
            else if(e.type == SDL_JOYAXISMOTION){
                if(e.jaxis.which == 0){
                    if(e.jaxis.axis == 0){
                        if(e.jaxis.value < -JOYSTICK_DEAD_ZONE){
                            xJoystickDir = -1;
                        }
                        else if(e.jaxis.value > JOYSTICK_DEAD_ZONE){
                            xJoystickDir =1;
                        }else{
                            xJoystickDir = 0;
                        }
                    }
                    //printf("Gamepad : %i\n", xJoystickDir);
                }
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
            player[0].move(1);
        }
        if(currentKeyStates[SDL_SCANCODE_A]){
            player[0].move(-1);
        }
        if(currentKeyStates[SDL_SCANCODE_SPACE]){
            player[0].jetpack();
            if(player1ParticleTimer.getTicks() >= 20){
                thrustParticles[currentThrustParticle].setPos(player[0].getX(), player[0].getY());
                thrustParticles[currentThrustParticle].reset();
                currentThrustParticle = (currentThrustParticle + 1) % THRUST_PARTICLE_NUMBER;
            }
        }

        if(currentKeyStates[SDL_SCANCODE_L]){
            player[1].move(1);
        }
        if(currentKeyStates[SDL_SCANCODE_J]){
            player[1].move(-1);
        }
        if(currentKeyStates[SDL_SCANCODE_K]){
            player[1].jetpack();
        }

        if (xJoystickDir == -1){
            player[0].move(-1);
        }
        if (xJoystickDir == 1){
            player[0].move(1);
        }

        if(gGameController && SDL_JoystickGetAttached(gGameController)){
            if(SDL_JoystickGetButton(gGameController, 0)){
                player[0].jetpack();
            }
        }

        SDL_SetRenderDrawColor(gRenderer, 135, 206, 235, 0xFF);
        SDL_RenderClear(gRenderer);

        gBGTexture.render(scrollingOffset, 0);
        gBGTexture.render(scrollingOffset + gBGTexture.getWidth(), 0);
        for(int i = 0; i < PLAYER_NUMBER; i++){
            if(i == 0) gSpriteSheetTexture.render(player[i].getX(), player[i].getY(), &gSpritesClips[0]);
            else gPlayer2Texture.render(player[i].getX(), player[i].getY(), &gPlayer2Clip);
            //player[i].drawCollider(gRenderer, &WHITE);
        }

        bool mortarOnScreen = (mortarX > -30) && mortarX < SCREEN_WIDTH;

        if(mortarOnScreen){
            if(!mortarProjectileTimer.isStarted() || mortarProjectileTimer.isPaused()){
                mortarProjectileTimer.start();
            }else if(mortarProjectileTimer.getTicks() >= 500){
                mortarProjectileTimer.start();
                projectiles[currentProj].setPos(mortarX, mortarY);
                projectiles[currentProj].setVelocity(-2*GLOBAL_SPEED, -GLOBAL_SPEED);
                if (currentProj++ > PROJECTILE_ARRAY_SIZE) currentProj = 0;
            }

            gMortar.render(mortarX, mortarY, NULL, 0.0, NULL, SDL_FLIP_HORIZONTAL);
        }

        mortarX -= GLOBAL_SPEED;

        for (int i = 0; i<PROJECTILE_ARRAY_SIZE; i++){ 
            projectiles[i].update();
            bool isInScreen = (
                    (projectiles[i].getX() > 0 - gProjectileRect.w) && 
                    (projectiles[i].getX() < SCREEN_WIDTH) && 
                    (projectiles[i].getY() > 0 - gProjectileRect.h) && 
                    (projectiles[i].getY() < SCREEN_HEIGHT)
                    );

            if (isInScreen){
                gProjectile.render(projectiles[i].getX(), projectiles[i].getY(), &gProjectileRect);
                //projectiles[i].drawCollider(gRenderer, &RED);

                for (int j = 0; j < PLAYER_NUMBER; j++)
                    if(Collision::collide(&projectiles[i].collider, &player[j].collider)){
                        if (collisionTimer.getTicks() > 500){
                            collisionTimer.start();
                            player[j].updateScore(-100);
                        }
                    }
            }
        }

        for (int i = 0; i < PLAYER_NUMBER; i++){
            player[i].update(deltaTime);
        }

        for (int i = 0; i < THRUST_PARTICLE_NUMBER; i++){
            thrustParticles[i].update(deltaTime);
        }

        if (scoreTimer.getTicks() >= 50){
            scoreTimer.start();
            for (int i = 0; i < PLAYER_NUMBER; i++){
                player[i].updateScore(1);
            }
            std::string scoreText = "Score Player 1 : " + std::to_string(player[0].getScore()) + "                          Player 2 : " + std::to_string(player[1].getScore());
            gScoreTexture.loadFromRenderedText(scoreText, WHITE, gScoreFont);
        }

        for(int i = 0; i < THRUST_PARTICLE_NUMBER; i++){
            thrustParticles[i].render(gRenderer);
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
