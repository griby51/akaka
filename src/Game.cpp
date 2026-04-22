#include "Game.hpp"
#include "CollisionSystem.hpp"
#include <SDL2/SDL_render.h>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <filesystem>

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
    mRenderer = renderer;
    mPlayerNumber = joinedCount;
    mWindow = window;

    SDL_RenderGetLogicalSize(renderer, &mScreenWidth, &mScreenHeight);
    
    mPlayers.resize(mPlayerNumber);
    mParticleTimers.resize(mPlayerNumber);
    mMissileTimers.resize(mPlayerNumber);

    for(const auto& entry : std::filesystem::directory_iterator("assets/hats/")){
        if(entry.path().extension() == ".png"){
            LTexture* tex = new LTexture();
            tex->setRenderer(mRenderer);
            tex->loadFromeFile(entry.path().string().c_str());
            printf("%s\n", entry.path().string().c_str());
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
        mPlayers[i].setKeyPreset(presets[playerSlot[i].presetIndex]);
        mPlayers[i].setSkin(skins[playerSlot[i].skinIndex]);
        printf("hat[0] ptr=%p width=%d height=%d\n", hats[0], hats[0]->getWidth(), hats[0]->getHeight());
        mPlayers[i].setHat(hats[playerSlot[i].hatIndex]);
        printf("Player %d hat index = %d, addr = %p\n", i, playerSlot[i].hatIndex, hats[playerSlot[i].hatIndex]);   
    }

    return true;
}

bool Game::loadMedia() {
    mSquirellTexture.setRenderer(mRenderer);
    mProjectileTexture.setRenderer(mRenderer);
    mDotTexture.setRenderer(mRenderer);
    mBGTexture.setRenderer(mRenderer);
    mScoreTexture.setRenderer(mRenderer);
    mMissileTexture.setRenderer(mRenderer);
    mCowboyTexture.setRenderer(mRenderer);

    mScoreFont = TTF_OpenFont("assets/Hypermonosaturation-zrMo0.ttf", 28);
    if (!mScoreFont) { printf("Font error: %s\n", TTF_GetError()); return false; }

    if (!mScoreTexture.loadFromRenderedText("Score : 0", mWhite, mScoreFont)){
        printf("Error loading score texture\n");
        return false;
    }
    if (!mCowboyTexture.loadFromeFile("assets/hats/cowboy_hat.png")){
        printf("Error loading cowboy hat\n");
        return false;
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

    mProjectileRect = {0, 0, 16, 16};
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
    mRandomBaseProjectileSpawnTicks = rand() % 3000;
    mRandomProjectilePos = rand() % mScreenHeight;

    for(int i = 0; i < mPlayerNumber; i++){
        mPlayers[i].setScreenSize(mScreenWidth, mScreenHeight);
        mPlayers[i].setPos(mScreenWidth / 2, mScreenHeight / 4);
        mPlayers[i].setCollider(0, 0, 32, 32);
        mMissileTimers[i].start();
        mParticleTimers[i].start();
    }

    for(int i = 0; i < MISSILE_NUMBER; i++){
        mMissiles[i].init(&mThrustParticleConfig, mMissileConfig);
        mMissiles[i].setPos(100000, 100000);
    }

    for (int i = 0; i < PROJECTILE_NUMBER; i++){
        mProjectiles[i].setPos(100000, 100000);
        mProjectiles[i].setCollider(0, 0, 16, 16);
    }

    mProjectileTimer.start();
    mCollisionTimer.start();
    mScoreTimer.start();
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
    }
}

void Game::update(float deltaTime){
    mDot.move();
    mScrollingOffset -= GLOBAL_SPEED;
    if(mScrollingOffset < -mBGTexture.getWidth()){
        mScrollingOffset = 0;
    }

    if(mProjectileTimer.getTicks() >= mRandomBaseProjectileSpawnTicks){
        mRandomBaseProjectileSpawnTicks = rand() % 1000;
        mRandomProjectilePos = rand() % mScreenHeight;
        mProjectileTimer.start();
        if(mCurrentProj >= PROJECTILE_NUMBER) mCurrentProj = 0;
        mProjectiles[mCurrentProj].setPos(mScreenWidth, mRandomProjectilePos);
        mProjectiles[mCurrentProj].setVelocity(-8, 0);
        mCurrentProj++;
    }

    for (int i = 0; i < PROJECTILE_NUMBER; i++){
        mProjectiles[i].update();
        bool inScreen = mProjectiles[i].getX() > -mProjectileRect.w &&
            mProjectiles[i].getX() < mScreenWidth &&
            mProjectiles[i].getY() > -mProjectileRect.h &&
            mProjectiles[i].getY() < mScreenHeight;

        mProjectiles[i].isInScreen = inScreen;

        if(inScreen){
            for(int j = 0; j < mPlayerNumber; j++){
                if(Collision::collide(&mProjectiles[i].collider, &mPlayers[j].collider)){
                    if(mCollisionTimer.getTicks() > 500){
                        mCollisionTimer.start();
                        mPlayers[j].updateScore(-100);
                    }
                }
            }
        }
    }

    for (int i = 0; i < mPlayerNumber; i++){
        mPlayers[i].update(deltaTime);
        for (int j = 0; j < MISSILE_NUMBER; j++){
            if(mMissiles[j].isAlive && Collision::collide(&mMissiles[j].collider, &mPlayers[i].collider)){
                mPlayers[i].updateScore(mMissileScorePenality);
                mMissiles[j].isAlive = false;
            }
        }
    }

    for(int i = 0; i < MISSILE_NUMBER; i++){
        mMissiles[i].update(deltaTime);
    }

    if(mScoreTimer.getTicks() >= 50){
        mScoreTimer.start();
        for(int i = 0; i < mPlayerNumber; i++){
            mPlayers[i].updateScore(1);
        }
    }
}

void Game::render(){
    SDL_SetRenderDrawColor(mRenderer, 135, 206, 235, 0xFF);
    SDL_RenderClear(mRenderer);

    mBGTexture.render(mScrollingOffset, 0);
    mBGTexture.render(mScrollingOffset + mBGTexture.getWidth(), 0);

    for (int i = 0; i < THRUST_PARTICLE_NUMBER; i++){
        mThrustParticles[i].render(mRenderer);
    }

    for (int i = 0; i < PROJECTILE_NUMBER; i++){
        if(mProjectiles[i].isInScreen){
            mProjectileTexture.render(mProjectiles[i].getX(), mProjectiles[i].getY(), &mProjectileRect);
        }
    }

    for (int i = 0; i < MISSILE_NUMBER; i++){
        if(mMissiles[i].isAlive){
            mMissileTexture.render(mMissiles[i].getX(), mMissiles[i].getY(), NULL, mMissiles[i].getAngleInDegree() + 90);
        }
        mMissiles[i].renderParticles(mRenderer);
    }

    std::string scoreText;
    SDL_Color colors[3] = {
        {100, 100, 255, 255},
        {255, 100, 100, 255},
        {100, 255, 100, 255}
    };

    for (int i = 0; i < mPlayerNumber; i++){
        mSquirellTexture.setColor(colors[i].r, colors[i].g, colors[i].b);
        mPlayers[i].render(mRenderer);
        scoreText += "Player " + std::to_string(i + 1) + " : " + std::to_string(mPlayers[i].getScore()) + "         ";
    }
    mScoreTexture.loadFromRenderedText(scoreText, mWhite, mScoreFont);
    mScoreTexture.render(20, 20);


    SDL_RenderPresent(mRenderer);
};

void Game::spawnMissile(int playerWhoSpawn){
    int target = playerWhoSpawn;
    while(target == playerWhoSpawn){
        target = rand() % mPlayerNumber;
    }
    mCurrentMissile = (mCurrentMissile + 1) % MISSILE_NUMBER;
    mMissiles[mCurrentMissile].reset();
    mMissiles[mCurrentMissile].setPos(mScreenWidth + 50, mScreenHeight / 2);
    mMissiles[mCurrentMissile].isAlive = true;
    mMissiles[mCurrentMissile].setTarget(&mPlayers[target].collider);
    
}

void Game::close() {
    mSquirellTexture.free();
    mProjectileTexture.free();
    mDotTexture.free();
    mBGTexture.free();
    mScoreTexture.free();
    mMissileTexture.free();
    mCowboyTexture.free();

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
