#include "Game.hpp"
#include "Explosion.hpp"
#include "LTexture.hpp"
#include "Missile.hpp"
#include "MissileManager.hpp"
#include "Player.hpp"
#include "ScoreCollectable.hpp"
#include "GameScene.hpp"
#include "TextureManager.hpp"
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

    playerManager.players.reserve(joinedCount);

    for(int i = 0; i < joinedCount; i++){
        player::PlayerConfig cfg;

        missile::MissileConfig missileCfg;
        
        explode::ExplosionConfig explosionConfig;
        explosionConfig.embers = true;
        explosionConfig.power = 2.f;

        missileCfg.particleConfig = mThrustParticleConfig;
        missileCfg.players = &playerManager.players;
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
        cfg.skin = TextureManager::getInstance().getTexture(playerSlot[i].skinId);
        cfg.hat = TextureManager::getInstance().getTexture(playerSlot[i].hatId);
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
    TextureManager& tm = TextureManager::getInstance();

    tm.loadTexture("pizza", "assets/collectables/pizza.png");
    tm.loadTexture("missile", "assets/missile00.png");
    tm.loadTexture("dot", "assets/dot.bmp");
    tm.loadTexture("bg", "assets/abstract_seamless_bg_01.png");
    
    tm.loadDirectory("assets/hats/", "hat_");
    tm.loadDirectory("assets/skins/", "skin_");

    int success = true;

    mScoreFont = TTF_OpenFont("assets/Hypermonosaturation-zrMo0.ttf", 14);
    if (!mScoreFont) {
        printf("Font error: %s\n", TTF_GetError());
        success = false; 
    }
    if (!mScoreTexture.loadFromRenderedText("Score : 0", mWhite, mScoreFont)){
        printf("Error loading score texture\n");
        success = false;
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

    mPizzaTimeUntilNext = rand() % 1000;
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
    if(mScrollingOffset < -TextureManager::getInstance().getTexture("bg")->getWidth()){
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
        mPizzaTimeUntilNext = rand() % 1000;
        mPizzaTimer.start();
        mPizza.emplace_back();
        mPizza.back().init(100, "pizza");
        mPizza.back().setPos(mScreenWidth, rand() % (mEffectiveHeight - 16));
        mPizza.back().vx = -GLOBAL_SPEED * 10;
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

    LTexture* bg = TextureManager::getInstance().getTexture("bg");

    bg->render(mScrollingOffset, 0);
    bg->render(mScrollingOffset + bg->getWidth(), 0);

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
    TextureManager::getInstance().clean();

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
