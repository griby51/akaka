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
#include "Missile.hpp"


GameConfig g_config("assets/config.ini");
int SCREEN_WIDTH = g_config.getInt("SCREEN_WIDTH", 800);
int SCREEN_HEIGHT = g_config.getInt("SCREEN_HEIGHT", 600);
bool missileEnabled = g_config.getBool("missileEnabled", true);
int PLAYER_NUMBER = g_config.getInt("PLAYER_NUMBER", 1);
int scoreToLaunchMissile = g_config.getInt("scoreToLaunchMissile", 100);
int missileScorePenality = g_config.getInt("missileScorePenality", -500);

GameConfig thrustParticleGameConfig("assets/playerThrustParticle.ini");
ParticleConfig thrustParticleConfig;

GameConfig missileConfig("assets/missileConfig.ini");

const int SCREEN_FPS = 60;
const int SCREEN_TICKS_PER_FRAME = 1000/SCREEN_FPS;

const int JOYSTICK_DEAD_ZONE = 8000;
const int THRUST_PARTICLE_NUMBER = 500;

const float GLOBAL_SPEED = 5;

bool init();
bool loadMedia();
void close();
void update(float deltaTime);
void render();

SDL_Window* gWindow = NULL;
SDL_Renderer* gRenderer = NULL;

SDL_Joystick* gGameController = NULL;
SDL_Rect gSpritesClips[2];
SDL_Rect gProjectileRect;
SDL_Rect gPlayer2Clip;
SDL_Rect gCactusManRect;

std::vector<Player> player(PLAYER_NUMBER);
ThrustParticle thrustParticles[THRUST_PARTICLE_NUMBER];
int randomBaseProjectileSpawnTicks = rand() % 3000;

LTexture gSpriteSheetTexture;
LTexture gPlayer2Texture;
LTexture gProjectile;
LTexture gDotTexture;
LTexture gBGTexture;
LTexture gScoreTexture;
LTexture gMissileTexture;

TTF_Font* gScoreFont = NULL;

SDL_Color RED = {255, 0, 0, 255};
SDL_Color WHITE = {255, 255, 255, 255};

const int PROJECTILE_ARRAY_SIZE = 20;
Projectile projectiles[PROJECTILE_ARRAY_SIZE];

void playerThrust(int playerNumber);
void spawnMissile(int playerWhoSpawn);

std::vector<LTimer> playerParticleTimer(PLAYER_NUMBER);
std::vector<LTimer> playerMissileTimer(PLAYER_NUMBER);

int currentThrustParticle = 0;
int randomProjectilePos = rand() % SCREEN_HEIGHT;

int scrollingOffset = 0;

Dot dot;
int currentProj = 0;

LTimer capTimer;
LTimer projectileTimer;
LTimer collisionTimer;
LTimer scoreTimer;
LTimer deltaTimer;

const int MISSILE_NUMBER = 50;
Missile missiles[MISSILE_NUMBER];
int currentMissile = 0;




std::stringstream timeText;
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

    gScoreFont = TTF_OpenFont("assets/Hypermonosaturation-zrMo0.ttf", 28);
    if(gScoreFont == NULL){
        printf("Failed to load font ! SDL_Error : %s", TTF_GetError());
        success = false;
    }else{
        if(!gScoreTexture.loadFromRenderedText("Score : 0", WHITE, gScoreFont)){
            printf("Failed to render text texure !\n");
            success = false;
        }
    }

    if(!gMissileTexture.loadFromeFile("assets/missile00.png")){
        printf("Failed to load missile texture !\n");
        success = false;
    }

    if(!gDotTexture.loadFromeFile("assets/dot.bmp")){
        printf("Failed to load dot texture\n");
        success = false;
    }

    if(!gBGTexture.loadFromeFile("assets/abstract_seamless_bg_01.png")){
        printf("Failed to load background texture\n");
        success = false;
    }

    if(!gProjectile.loadFromeFile("assets/cannonbob.png")){
        printf("Failed to laod cannonbob texture");
        success = false;
    }else{
        gProjectileRect.x = 0;
        gProjectileRect.y = 0;
        gProjectileRect.w = 16;
        gProjectileRect.h = 16;
    }



    if(!gSpriteSheetTexture.loadFromeFile("assets/player.png")){
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

    if(!gPlayer2Texture.loadFromeFile("assets/dawntree.png")){
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
    gProjectile.setRenderer(gRenderer);
    gDotTexture.setRenderer(gRenderer);
    gBGTexture.setRenderer(gRenderer);
    gScoreTexture.setRenderer(gRenderer);
    gMissileTexture.setRenderer(gRenderer);

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
    for (int i = 0; i < MISSILE_NUMBER; i++){
        missiles[i].init(&thrustParticleConfig, missileConfig);
        missiles[i].setPos(10000, 10000);
    }

    SDL_Event e;

    // LTimer fpsTimer;

    scoreTimer.start();

    for (int i = 0; i < PLAYER_NUMBER; i++){
        player[i].setPos(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 4);
        playerMissileTimer[i].start();
    }

    for(int i = 0; i < PLAYER_NUMBER; i++){
        player[i].setCollider(0, 0, 32, 32);
        playerParticleTimer[i].start();
    }


    for (int i = 0; i < PROJECTILE_ARRAY_SIZE; i++){
        projectiles[i].setPos(10000, 10000);
        projectiles[i].setCollider(0, 0, 16, 16);
    };

    projectileTimer.start();
    collisionTimer.start();


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


        // float avgFPS = countedFrames / (fpsTimer.getTicks() / 1000.f);
        // if (avgFPS > 2000000) avgFPS = 0;
        // // printf("FPS : %g\n", avgFPS);


        const Uint8* currentKeyStates = SDL_GetKeyboardState(NULL);
        if(currentKeyStates[SDL_SCANCODE_D]){
            player[0].move(1);
        }
        if(currentKeyStates[SDL_SCANCODE_A]){
            player[0].move(-1);
        }
        if(currentKeyStates[SDL_SCANCODE_W]){
            if(player[0].getScore() >= scoreToLaunchMissile && playerMissileTimer[0].getTicks() > 500){
                player[0].updateScore(-scoreToLaunchMissile);
                playerMissileTimer[0].start();
                spawnMissile(0);
            }
        }
        if(currentKeyStates[SDL_SCANCODE_SPACE]){
            playerThrust(0);
        }

        if(PLAYER_NUMBER > 1){
            if(currentKeyStates[SDL_SCANCODE_L]){
                player[1].move(1);
            }
            if(currentKeyStates[SDL_SCANCODE_J]){
                player[1].move(-1);
            }
            if(currentKeyStates[SDL_SCANCODE_I]){
                if(player[1].getScore() >= scoreToLaunchMissile && playerMissileTimer[1].getTicks() > 500){
                    player[1].updateScore(-scoreToLaunchMissile);
                    playerMissileTimer[1].start();
                    spawnMissile(1);
                }
            }
            if(currentKeyStates[SDL_SCANCODE_K]){
                playerThrust(1);
            }
        }

        if(PLAYER_NUMBER > 2){
            if(currentKeyStates[SDL_SCANCODE_KP_6]){
                player[2].move(1);
            }
            if(currentKeyStates[SDL_SCANCODE_KP_4]){
                player[2].move(-1);
            }
            if(currentKeyStates[SDL_SCANCODE_KP_8]){
                if(player[2].getScore() >= scoreToLaunchMissile && playerMissileTimer[2].getTicks() > 500){
                    player[2].updateScore(-scoreToLaunchMissile);
                    playerMissileTimer[2].start();
                    spawnMissile(2);
                }
             }
            if(currentKeyStates[SDL_SCANCODE_KP_5]){
                playerThrust(2);
            }
        }

        if (xJoystickDir == -1){
            player[0].move(-1);
        }
        if (xJoystickDir == 1){
            player[0].move(1);
        }

        if(gGameController && SDL_JoystickGetAttached(gGameController)){
            if(SDL_JoystickGetButton(gGameController, 0)){
                playerThrust(0);
            }
        }
        update(deltaTime);
        render();

        deltaTimer.start();

        int frameTicks = capTimer.getTicks();
        if(frameTicks < SCREEN_TICKS_PER_FRAME){
            SDL_Delay(SCREEN_TICKS_PER_FRAME - frameTicks);
        }
    }

    close();

    return 0;
}

void playerThrust(int playerNumber){
    player[playerNumber].jetpack();
    if(playerParticleTimer[playerNumber].getTicks() >= 20){
        playerParticleTimer[playerNumber].start();
        thrustParticles[currentThrustParticle].setPos(player[playerNumber].getX() + 5, player[playerNumber].getY() + 25);
        thrustParticles[currentThrustParticle].reset();
        currentThrustParticle = (currentThrustParticle + 1) % THRUST_PARTICLE_NUMBER;
    }
}

void update(float deltaTime){

    dot.move();

    scrollingOffset -= GLOBAL_SPEED;

    if (scrollingOffset < -gBGTexture.getWidth()){
            scrollingOffset = 0;
    }
    if(projectileTimer.getTicks() >= randomBaseProjectileSpawnTicks){
        randomBaseProjectileSpawnTicks = rand() % 1000;
        randomProjectilePos = rand() % SCREEN_HEIGHT;
        projectileTimer.start();
        if (currentProj >= PROJECTILE_ARRAY_SIZE) currentProj = 0;
        projectiles[currentProj].setPos(SCREEN_WIDTH, randomProjectilePos);
        projectiles[currentProj].setVelocity(-8, 0);
        currentProj++;
    }
    for (int i = 0; i<PROJECTILE_ARRAY_SIZE; i++){ 
        projectiles[i].update();
        bool isInScreen = (
                (projectiles[i].getX() > 0 - gProjectileRect.w) && 
                (projectiles[i].getX() < SCREEN_WIDTH) && 
                (projectiles[i].getY() > 0 - gProjectileRect.h) && 
                (projectiles[i].getY() < SCREEN_HEIGHT)
            );
        projectiles[i].isInScreen = isInScreen;

        if (isInScreen){

            for (int j = 0; j < PLAYER_NUMBER; j++){
                if(Collision::collide(&projectiles[i].collider, &player[j].collider)){
                    if (collisionTimer.getTicks() > 500){
                        collisionTimer.start();
                        player[j].updateScore(-100);
                    }
                }
            }
        }
    }

    for (int i = 0; i < PLAYER_NUMBER; i++){
        player[i].update(deltaTime);
        for(int j = 0; j < MISSILE_NUMBER; j++){
            if(missiles[j].isAlive){
                if(Collision::collide(&missiles[j].collider, &player[i].collider)){
                    player[i].updateScore(missileScorePenality);
                    missiles[j].isAlive = false;
                }
            }
        }
    }

    for (int i = 0; i < THRUST_PARTICLE_NUMBER; i++){
        thrustParticles[i].update(deltaTime);
    }

    for (int i = 0; i < MISSILE_NUMBER; i++){
        missiles[i].update(deltaTime);
    }

    if (scoreTimer.getTicks() >= 50){
        scoreTimer.start();
        for (int i = 0; i < PLAYER_NUMBER; i++){
            player[i].updateScore(1);
        }
    }
}

void render(){

    SDL_SetRenderDrawColor(gRenderer, 135, 206, 235, 0xFF);
    SDL_RenderClear(gRenderer);

    gBGTexture.render(scrollingOffset, 0);
    gBGTexture.render(scrollingOffset + gBGTexture.getWidth(), 0);

    for(int i = 0; i < THRUST_PARTICLE_NUMBER; i++){
        thrustParticles[i].render(gRenderer);
    }

    for(int i = 0; i < PROJECTILE_ARRAY_SIZE; i++){
        if (projectiles[i].isInScreen){
            gProjectile.render(projectiles[i].getX(), projectiles[i].getY(), &gProjectileRect);
        }    
    }

    for(int i = 0; i < MISSILE_NUMBER; i++){
        if(missiles[i].isAlive){
            gMissileTexture.render(missiles[i].getX(), missiles[i].getY(), NULL, missiles[i].getAngleInDegree() + 90);
        }
        missiles[i].renderParticles(gRenderer);
    }

    std::string scoreText="";
    for(int i = 0; i < PLAYER_NUMBER; i++){
        if(i == 0){
            gSpriteSheetTexture.setColor(100, 100, 255);
        }
        else if(i == 1){
            gSpriteSheetTexture.setColor(255, 100, 100);
        }else if(i == 2){
            gSpriteSheetTexture.setColor(100, 255, 100);
        }
        gSpriteSheetTexture.render(player[i].getX(), player[i].getY(), &gSpritesClips[0]);
        scoreText += "Player " + std::to_string((i+1)) + " : " + std::to_string(player[i].getScore()) + "        ";
    }
    gScoreTexture.loadFromRenderedText(scoreText, WHITE, gScoreFont);  
    gScoreTexture.render(20, 20);

    SDL_RenderPresent(gRenderer);
}

void spawnMissile(int playerWhoSpawn){
    int targetPlayer = playerWhoSpawn;
    while(targetPlayer == playerWhoSpawn){
        targetPlayer = rand() % PLAYER_NUMBER;
    }
    currentMissile++;
    if(currentMissile >= MISSILE_NUMBER) currentMissile = 0;
    missiles[currentMissile].reset();
    missiles[currentMissile].setPos(SCREEN_WIDTH + 50, SCREEN_HEIGHT / 2);
    missiles[currentMissile].isAlive = true;
    missiles[currentMissile].setTarget(&player[targetPlayer].collider);
}
