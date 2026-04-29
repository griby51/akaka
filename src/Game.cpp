#include "Game.hpp"
#include "CollisionSystem.hpp"
#include "ScoreCollectable.hpp"
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
    mThrustParticleGameConfig("assets/playerThrustParticle.ini"),
    mMissileConfig("assets/missileConfig.ini")
{
    mScreenWidth = mConfig.getInt("SCREEN_WIDTH", 800);
    mScreenHeight = mConfig.getInt("SCREEN_HEIGHT", 600);
    mPlayerNumber = mConfig.getInt("PLAYER_NUMBER", 2);
    mScoreToLaunchMissile = mConfig.getInt("scoreToLaunchMissile", 100);
    mMissileScorePenality = mConfig.getInt("missileScorePenality", -500);
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
    
    mPlayers.resize(mPlayerNumber);
    mParticleTimers.resize(mPlayerNumber);
    mMissileTimers.resize(mPlayerNumber);

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


    for(int i = 0; i < mPlayerNumber; i++){
        mPlayers[i].init(&mConfig, i);
        mPlayers[i].setPlayerTable(mPlayers.data(), mPlayerNumber);
        mPlayers[i].setMissileTable(mMissiles, MISSILE_NUMBER, &mCurrentMissile);
        if(playerSlot[i].presetIndex >= 0){
            mPlayers[i].setKeyPreset(presets[playerSlot[i].presetIndex]);
        }
        mPlayers[i].setSkin(skins[playerSlot[i].skinIndex]);
        mPlayers[i].setHat(hats[playerSlot[i].hatIndex]);
        mPlayers[i].setJoystickId(playerSlot[i].joystickId);
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
    
    gFireLoop = Mix_LoadWAV("assets/sounds/sfx/fireLoop.wav");
    if(gFireLoop == NULL){
        printf("Failed to laod fire loop sound effect! Error : %s\n", Mix_GetError());
        return false;
    }

    gJetpackThrustSFX = Mix_LoadWAV("assets/sounds:sfx/jetpackThrust.wav");
    //https://opengameart.org/content/engine-loop-heavy-vehicletank
    if(gJetpackThrustSFX == NULL){
        printf("Failed to load jetpackThrust SFX : %S\n", Mix_GetError());
    }

    return true;
}

void Game::start(){

    for(int i = 0; i < mPlayerNumber; i++){
        mPlayers[i].setParticleConfig(mThrustParticleGameConfig);
    }

    SDL_SetRenderDrawBlendMode(mRenderer, SDL_BLENDMODE_BLEND);

    mThrustParticleConfig.load(mThrustParticleGameConfig);
    for (int i = 0; i < THRUST_PARTICLE_NUMBER; i++){
        mThrustParticles[i].init(&mThrustParticleConfig);
    }

    srand(time(0));

    for(int i = 0; i < mPlayerNumber; i++){
        mPlayers[i].setScreenSize(mScreenWidth, mEffectiveHeight);
        mPlayers[i].setPos(mScreenWidth / 2, mEffectiveHeight / 4);
        mPlayers[i].setCollider(0, 0, 32, 32);
        mMissileTimers[i].start();
        mParticleTimers[i].start();
    }

    for(int i = 0; i < MISSILE_NUMBER; i++){
        mMissiles[i].init(&mThrustParticleConfig, mMissileConfig);
        mMissiles[i].setPos(100000, 100000);
    }

    mPizzaTimeUntilNext = rand() % 10000;
    printf("pizzaTimer delay : %i\n", mPizzaTimeUntilNext);
    mPizzaTimer.start();
}

void Game::handleEvents(const SDL_Event& e) {
    if (e.type == SDL_QUIT){
        mQuit = true;
        return;
    }
    if (e.type == SDL_JOYAXISMOTION && e.jaxis.which == 0 && e.jaxis.axis == 0) {
        if(e.jaxis.value < -JOYSTICK_DEAD_ZONE){
            mXJoystickDir = -1;
        }
        else if (e.jaxis.value >  JOYSTICK_DEAD_ZONE){
            mXJoystickDir =  1;
        }
        else{
            mXJoystickDir =  0;
        }
    }
}
void Game::handleInput(){
    const Uint8* keys = SDL_GetKeyboardState(NULL);
    for (int i = 0; i < mPlayerNumber; i++){
        mPlayers[i].handleInput(keys);

        int joyId = mPlayers[i].getJoystickId();

        if (joyId >= 0){
            SDL_Joystick* joy = SDL_JoystickFromInstanceID(joyId);
            mPlayers[i].handleJoystickInput(joy);
        }
    }
}

void Game::update(float deltaTime){
    mDot.move();
    mScrollingOffset -= GLOBAL_SPEED * deltaTime;
    if(mScrollingOffset < -mBGTexture.getWidth()){
        mScrollingOffset = 0;
    }

    for (int i = 0; i < mPlayerNumber; i++){
        mPlayers[i].update(deltaTime);
        for (int j = 0; j < MISSILE_NUMBER; j++){
            if(mMissiles[j].isAlive && Collision::collide(&mMissiles[j].collider, &mPlayers[i].collider)){
                mPlayers[i].updateLife(-25);
                mMissiles[j].isAlive = false;
            }
        }

        if(mPlayers[i].getScore() < 0){
            mPlayers[i].updateScore(-mPlayers[i].getScore());
        }
    }

    for(int i = 0; i < MISSILE_NUMBER; i++){
        mMissiles[i].update(deltaTime);
    }

    mPizza.erase(
            std::remove_if(mPizza.begin(), mPizza.end(),
                [](const ScoreCollectable& col){
                return !col.isAlive;
                }),
            mPizza.end()
            );

    for(int i = 0; i < mPizza.size(); i++){
        mPizza[i].update(deltaTime, &mPlayers);
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

    for (int i = 0; i < mPlayerNumber; i++){
        mPlayers[i].render(mRenderer);
    }

    for (int i = 0; i < MISSILE_NUMBER; i++){
        if(mMissiles[i].isAlive){
            mMissileTexture.render(mMissiles[i].getX(), mMissiles[i].getY(), NULL, mMissiles[i].getAngleInDegree() + 90);

            Mix_PlayChannel(-1, gFireLoop, 0);
        }
        mMissiles[i].renderParticles(mRenderer);
    }

    for (int i = 0; i < mPizza.size(); i++){
        mPizza[i].render(mRenderer);
    }


    SDL_Rect indicatorRect;
    indicatorRect.w = mScreenWidth / mPlayerNumber;
    indicatorRect.h = 50;
    indicatorRect.y = mScreenHeight - indicatorRect.h;


    for(int i = 0; i < mPlayerNumber; i++){
        Uint8 greyIntensity = i*20 + 150;
        std::string playerNumber = "Player " + std::to_string(i + 1);
        std::string score = std::to_string(mPlayers[i].getScore());
        std::string life = std::to_string(mPlayers[i].getLife());

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
        mPlayers[i].getSkin()->render((i + 1) * indicatorRect.w - 42, indicatorRect.y + 10);
        mPlayers[i].getHat()->render((i + 1) * indicatorRect.w - 42, indicatorRect.y + 10);

        
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
