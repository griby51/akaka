#include "Game.hpp"
#include "Explosion.hpp"
#include "Missile.hpp"
#include "MissileManager.hpp"
#include "Player.hpp"
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
    mThrustParticleConfig.load(mThrustParticleGameConfig);

    mRenderer = renderer;
    mPlayerNumber = joinedCount;
    mWindow = window;
    
    audioManager.init();

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

    playerManager.players.reserve(joinedCount);

    for(int i = 0; i < joinedCount; i++){
        player::PlayerConfig cfg;

        missile::MissileConfig missileCfg;
        
        explode::ExplosionConfig explosionConfig;
        explosionConfig.embers = true;
        explosionConfig.power = 2.f;

        missileCfg.particleConfig = mThrustParticleConfig;
        missileCfg.players = &playerManager.players;
        missileCfg.texture = &mMissileTexture;
        missileCfg.explosionConfig = explosionConfig;
        missileCfg.explosionManager = &explosionManager;
        missileCfg.audioManager = &audioManager;

        missileCfg.precision = mConfig.getFloat("missile_precision", 3.f);
        missileCfg.velocity = mConfig.getFloat("missile_velocity", 1000.0f);
        missileCfg.explosionTriggerRange = mConfig.getFloat("missile_explosion_trigger_range", 70.f);
        missileCfg.showCollider = mConfig.getBool("show_missile_collider", false);
        missileCfg.explosionDelay = mConfig.getInt("missile_explosion_delay", 70);
        missileCfg.maxDamage = mConfig.getFloat("missile_max_dmg", 40.f);

        cfg.players = &playerManager.players;
        cfg.skin = skins[playerSlot[i].skinIndex];
        cfg.hat = hats[playerSlot[i].hatIndex];
        cfg.audioManager = &audioManager;

        cfg.jetpackForce = mConfig.getFloat("player_jetpack_force", 700.f);
        cfg.maxVx = mConfig.getFloat("player_max_vx", 1000.f);
        cfg.acceleration = mConfig.getFloat("player_acceleration", 1000.f);
        cfg.deceleration = mConfig.getFloat("player_deceleration", 0.8f);
        cfg.maxHealth = mConfig.getInt("player_health", 100);
        cfg.bounce = mConfig.getBool("player_bounce", true);
        cfg.bounceRestitution = mConfig.getFloat("bounce_restitution", 0.4f);
        cfg.scoreToLaunchMissile = mConfig.getInt("score_to_launch_missile", 200);
        cfg.showCollider = mConfig.getBool("show_player_collider", false);
        cfg.gravityForce = mConfig.getFloat("gravity", -500.f);
        

        if(playerSlot[i].presetIndex >= 0){
            cfg.keyPreset = presets[playerSlot[i].presetIndex];
        }
        cfg.joystickId = playerSlot[i].joystickId;
        cfg.thrustParticleConfig = mThrustParticleConfig;
        cfg.screenWidth = mScreenWidth;
        cfg.screenHeight = mEffectiveHeight;
        cfg.missileManager = &mMissileManager;
        cfg.missileConfig = missileCfg;

        playerManager.addPlayer(cfg);
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

    audioManager.loadSFX("jetpackThrust", "assets/sounds/sfx/jetpackThrust.wav");
    audioManager.loadSFX("missileLaunch", "assets/sounds/sfx/rocket_launch_1.wav");
    audioManager.loadSFX("explosion", "assets/sounds/sfx/synthetic_explosion_1.wav");
    audioManager.loadSFX("boing", "assets/sounds/sfx/boiiing.wav");

    audioManager.loadMusic("miniloop14", "assets/sounds/musics/22PurgatoryPackMiniLoop14.ogg");

    return true;
}

void Game::start(){

    SDL_SetRenderDrawBlendMode(mRenderer, SDL_BLENDMODE_BLEND);

    audioManager.playMusic("miniloop14");
    audioManager.setMusicVolume(32);

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
    int offsetX = explosionManager.getShakeX();
    int offsetY = explosionManager.getShakeY();

    SDL_Rect viewport = {offsetX, offsetY, mScreenWidth, mScreenHeight};

    SDL_RenderSetViewport(mRenderer, &viewport);

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

    SDL_RenderSetViewport(mRenderer, NULL);


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
