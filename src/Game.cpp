#include "Game.hpp"
#include "Explosion.hpp"
#include "MissileManager.hpp"
#include "ScoreCollectable.hpp"
#include "GameScene.hpp"
#include <SDL2/SDL_joystick.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_render.h>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <algorithm>

Game::Game()
    : mConfig("assets/config.ini"),
    mThrustParticleGameConfig("assets/playerThrustParticle.ini")
{
    mScreenWidth = mConfig.getInt("SCREEN_WIDTH", 800);
    mScreenHeight = mConfig.getInt("SCREEN_HEIGHT", 600);
    mPlayerNumber = mConfig.getInt("PLAYER_NUMBER", 2);
}

Game::~Game() {
    close();
}

bool Game::init(SDL_Renderer* renderer, SDL_Window* window, PlayerSlot* playerSlot, int joinedCount){
    srand(time(NULL));

    mRenderer = renderer;
    mPlayerNumber = joinedCount;
    mWindow = window;

    SDL_RenderGetLogicalSize(mRenderer, &mScreenWidth, &mScreenHeight);

    mEffectiveHeight = mScreenHeight - 50;

    for(const auto& entry : std::filesystem::directory_iterator("assets/hats/")){
        if(entry.path().extension() == ".png"){
            LTexture* tex = new LTexture();
            tex->setRenderer(mRenderer);
            tex->loadFromeFile(entry.path().string().c_str());
            hats.push_back(tex);
        }
    }

    for(const auto& entry : std::filesystem::directory_iterator("assets/skins/")){
        if(entry.path().extension() == ".png"){
            LTexture* tex = new LTexture();
            tex->setRenderer(mRenderer);
            tex->loadFromeFile(entry.path().string().c_str());
            skins.push_back(tex);
        }
    }

    return true;
}

bool Game::loadMedia() {
    int success = true;

    mSquirellTexture.setRenderer(mRenderer);
    mProjectileTexture.setRenderer(mRenderer);
    mDotTexture.setRenderer(mRenderer);
    mBGTexture.setRenderer(mRenderer);
    mScoreTexture.setRenderer(mRenderer);
    mMissileTexture.setRenderer(mRenderer);
    mCowboyTexture.setRenderer(mRenderer);
    mPizzaTexture.setRenderer(mRenderer);

    mScoreFont = TTF_OpenFont("assets/Hypermonosaturation-zrMo0.ttf", 14);
    if (!mScoreFont) {
        printf("Font error: %s\n", TTF_GetError());
        success = false; 
    }
    if(!mPizzaTexture.loadFromeFile("assets/collectables/pizza.png")){
        printf("Error loading pizza texture\n");
        success = false;
    }
    if (!mScoreTexture.loadFromRenderedText("Score : 0", mWhite, mScoreFont)){
        printf("Error loading score texture\n");
        success = false;
    }
    if (!mMissileTexture.loadFromeFile("assets/missile00.png")){
        printf("Error loading missile texture\n");
        return false;
    }
    if (!mDotTexture.loadFromeFile("assets/dot.bmp")){
        printf("Error loading dot texture\n");
        return false;
    }
    if (!mBGTexture.loadFromeFile("assets/abstract_seamless_bg_01.png")){
        printf("Error loading background\n");
        return false;
    }
    if (!mProjectileTexture.loadFromeFile("assets/cannonbob.png")){
        printf("Error loading projectile texture\n");
        return false;
    }
    if (!mSquirellTexture.loadFromeFile("assets/skins/squirell.png")){
        printf("Error loading squirell texture");
        return false;
    }  
    gFireLoop = Mix_LoadWAV("assets/sounds/sfx/jetpackThrust.wav");
    if(gFireLoop == NULL){
        printf("Failed to laod fire loop sound effect! Error : %s\n", Mix_GetError());
        return false;
    }
    gJetpackThrustSFX = Mix_LoadWAV("assets/sounds/sfx/jetpackThrust.wav");
    //https://opengameart.org/content/engine-loop-heavy-vehicletank
    if(gJetpackThrustSFX == NULL){
        //printf("Failed to load jetpackThrust SFX : %s\n", Mix_GetError());
    }
    gMissileLaunchSFX = Mix_LoadWAV("assets/sounds/sfx/rocket_launch_1.wav");
    if(gMissileLaunchSFX == NULL){
        printf("Failed to load missile launch SFX : %s\n", Mix_GetError());
    }

    return true;
}

void Game::start(){

    SDL_SetRenderDrawBlendMode(mRenderer, SDL_BLENDMODE_BLEND);

    mThrustParticleConfig.load(mThrustParticleGameConfig);
    for (int i = 0; i < THRUST_PARTICLE_NUMBER; i++){
        mThrustParticles[i].init(&mThrustParticleConfig);
    }

    srand(time(0));

    mPizzaTimeUntilNext = rand() % 10000;
    printf("pizzaTimer delay : %i\n", mPizzaTimeUntilNext);
    mPizzaTimer.start();
}

void Game::handleEvents(const SDL_Event& e) {
    if (e.type == SDL_QUIT){
        mQuit = true;
        return;
    }
}

void Game::update(float deltaTime){
    mDot.move();
    mScrollingOffset -= GLOBAL_SPEED * deltaTime;
    if(mScrollingOffset < -mBGTexture.getWidth()){
        mScrollingOffset = 0;
    }

    mMissileManager.update(deltaTime);
    explosionManager.update(deltaTime);
    playerManager.update(deltaTime);

    mPizza.erase(
            std::remove_if(mPizza.begin(), mPizza.end(),
                [](const ScoreCollectable& col){
                return !col.isAlive;
                }),
            mPizza.end()
            );

    for(int i = 0; i < mPizza.size(); i++){
        mPizza[i].update(deltaTime, &playerManager.players);
    }

    if (mPizzaTimer.getTicks() > mPizzaTimeUntilNext){
        mPizzaTimeUntilNext = rand() % 10000;
        mPizzaTimer.start();
        mPizza.emplace_back();
        mPizza.back().init(100, &mPizzaTexture);
        mPizza.back().setPos(mScreenWidth, rand() % (mEffectiveHeight - 16));
        mPizza.back().vx = -GLOBAL_SPEED;
        mPizza.back().collider.w = 16;
        mPizza.back().collider.h = 16;
    }
}

void Game::render(){
    SDL_SetRenderDrawColor(mRenderer, 135, 206, 235, 0xFF);
    SDL_RenderClear(mRenderer);

    mBGTexture.render(mScrollingOffset, 0);
    mBGTexture.render(mScrollingOffset + mBGTexture.getWidth(), 0);

    explosionManager.render(mRenderer);
    playerManager.render(mRenderer);

    for (int i = 0; i < mPizza.size(); i++){
        mPizza[i].render(mRenderer);
    }


    SDL_Rect indicatorRect;
    indicatorRect.w = mScreenWidth / mPlayerNumber;
    indicatorRect.h = 50;
    indicatorRect.y = mScreenHeight - indicatorRect.h;

    mMissileManager.render(mRenderer);


    for(int i = 0; i < playerManager.players.size(); i++){
        Uint8 greyIntensity = i*20 + 150;
        std::string playerNumber = "Player " + std::to_string(i + 1);
        std::string score = std::to_string(playerManager.players[i].getScore());
        std::string life = std::to_string(playerManager.players[i].getLife());

        LTexture scoreTexture;
        LTexture lifeTexture;
        LTexture playerNumberTexture;

        scoreTexture.setRenderer(mRenderer);
        lifeTexture.setRenderer(mRenderer);
        playerNumberTexture.setRenderer(mRenderer);

        playerNumberTexture.loadFromRenderedText(playerNumber, mWhite, mScoreFont);
        lifeTexture.loadFromRenderedText(life, mRed, mScoreFont);
        scoreTexture.loadFromRenderedText(score, mGreen, mScoreFont);

        indicatorRect.x = i * indicatorRect.w;
        SDL_SetRenderDrawColor(mRenderer, greyIntensity, greyIntensity, greyIntensity, 255);

        SDL_RenderFillRect(mRenderer, &indicatorRect);
        playerManager.players[i].getSkin()->render((i + 1) * indicatorRect.w - 42, indicatorRect.y + 10);
        playerManager.players[i].getHat()->render((i + 1) * indicatorRect.w - 42, indicatorRect.y + 10);

        playerNumberTexture.render(indicatorRect.x, indicatorRect.y);
        lifeTexture.render(indicatorRect.x, indicatorRect.y + playerNumberTexture.getHeight() + 10);
        scoreTexture.render(indicatorRect.x + lifeTexture.getWidth() + 10, indicatorRect.y + playerNumberTexture.getHeight() + 10);
    }

    SDL_RenderPresent(mRenderer);
};

void Game::close() {
    mSquirellTexture.free();
    mProjectileTexture.free();
    mDotTexture.free();
    mBGTexture.free();
    mScoreTexture.free();
    mMissileTexture.free();
    mCowboyTexture.free();

    Mix_FreeChunk(gFireLoop);
    gFireLoop = NULL;

    if (mScoreFont){
        TTF_CloseFont(mScoreFont); mScoreFont = nullptr;
    }
    if (mController){
        SDL_JoystickClose(mController); mController = nullptr;
    }
}

bool Game::isOver(){
    return mQuit;
}
