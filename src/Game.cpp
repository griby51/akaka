#include "Game.hpp"
#include "Ability.hpp"
#include "Christmas.hpp"
#include "Explosion.hpp"
#include "LTexture.hpp"
#include "Missile.hpp"
#include "Player.hpp"
#include "ScoreCollectable.hpp"
#include "TextureManager.hpp"
#include "AnimationManager.hpp"
#include "TrafficCone.hpp"
#include "ScriptEngine.hpp"
#include "Utils.hpp"
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_joystick.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_render.h>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <algorithm>
#include <memory>

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
    
    audioManager.init();

    mContext.effectManager = &effectManager;
    mContext.audioManager = &audioManager;
    mContext.effectiveHeight = &mEffectiveHeight; 
    mContext.screenWidth = &mScreenWidth;
    mContext.screenHeight = &mScreenHeight;
    mContext.players = &playerManager.players;
    mContext.particleManager = &particleManager;


    SDL_RenderGetLogicalSize(mRenderer, &mScreenWidth, &mScreenHeight);

    mEffectiveHeight = mScreenHeight - 50;

    playerManager.players.reserve(joinedCount);

    mThrustParticleConfig.load(mThrustParticleGameConfig);

    projectile::MissileConfig missileCfg;
    
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
    missileCfg.particleManager = &particleManager;

    for(int i = 0; i < joinedCount; i++){
        player::PlayerConfig cfg;

        cfg.players = &playerManager.players;
        cfg.skin = TextureManager::getInstance().getTexture(playerSlot[i].skinId);
        cfg.hat = TextureManager::getInstance().getTexture(playerSlot[i].hatId);
        cfg.skinId = playerSlot[i].skinId;
        cfg.hatId = playerSlot[i].hatId;
        cfg.audioManager = &audioManager;
        cfg.particleManager = &particleManager;

        cfg.ability = ScriptEngine::getInstance().createAbilityForHat(playerSlot[i].hatId, &mContext);

        if(!cfg.ability){
            if(playerSlot[i].hatId == "hat_trafficCone"){
                projectile::TrafficConeConfig trafficConeCfg;
                explode::ExplosionConfig eCfg;
                eCfg.power = 4.f;
                trafficConeCfg.explosionConfig = eCfg;
                trafficConeCfg.explosionManager = &explosionManager;
                trafficConeCfg.globalSpeed = &GLOBAL_SPEED;
                trafficConeCfg.triggerRange = 20.f;
                trafficConeCfg.speed = 1500.f;
                trafficConeCfg.explosionTriggerRange = 100.f;
                trafficConeCfg.audioManager = &audioManager;
                trafficConeCfg.particleConfig = mThrustParticleConfig;
                trafficConeCfg.players = &playerManager.players;
                trafficConeCfg.particleManager = &particleManager;
                cfg.ability = std::make_unique<TrafficConeAbility>(&projectileManager, trafficConeCfg, mScreenWidth, mEffectiveHeight);
            }else if(playerSlot[i].hatId == "hat_christmas"){
                projectile::ChristmasSleighConfig sleighCfg;
                sleighCfg.players = &playerManager.players;
                sleighCfg.projectileManager = &projectileManager;
                sleighCfg.audioManager = &audioManager;
                sleighCfg.screenWidth = mScreenWidth;
                sleighCfg.screenHeight = mEffectiveHeight;

                explode::ExplosionConfig eCfg;
                eCfg.power = 2.f;

                sleighCfg.giftConfig.players = &playerManager.players;
                sleighCfg.giftConfig.explosionManager = &explosionManager;
                sleighCfg.giftConfig.explosionConfig = eCfg;
                sleighCfg.giftConfig.audioManager = &audioManager;

                cfg.ability = std::make_unique<ChristmasSleighAbility>(&projectileManager, sleighCfg);
            }else{
                cfg.ability = std::make_unique<MissileAbility>(&projectileManager, missileCfg, mScreenWidth, mScreenHeight);
            }
        }

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

        playerManager.addPlayer(std::move(cfg));
    }

    return true;
}

bool Game::loadMedia() {
    TextureManager& tm = TextureManager::getInstance();
    
    tm.loadDirectory("assets/hats/", "hat_");
    tm.loadDirectory("assets/skins/", "skin_");

    int success = true;

    AnimationManager& am = AnimationManager::getInstance();

    Animation missileAnim{"explosion_missile_sheet", 64, 64, 6, 30, 0.03, false};
    am.registerAnimation("explosion_missile", missileAnim);

    mScoreFont = TTF_OpenFont("assets/pixelfont.ttf", 14);
    if (!mScoreFont) {
        printf("Font error: %s\n", TTF_GetError());
        success = false; 
    }

    audioManager.loadSFX("jetpackThrust", "assets/sounds/sfx/jetpackThrust.wav");
    audioManager.loadSFX("missileLaunch", "assets/sounds/sfx/rocket_launch_1.wav");
    audioManager.loadSFX("explosion", "assets/sounds/sfx/synthetic_explosion_1.wav");
    audioManager.loadSFX("boing", "assets/sounds/sfx/boiiing.wav");

    audioManager.loadMusic("miniloop14", "assets/sounds/musics/22PurgatoryPackMiniLoop14.ogg");

    return success;
}

void Game::start(){

    SDL_SetRenderDrawBlendMode(mRenderer, SDL_BLENDMODE_BLEND);

    if(mConfig.getBool("music", true)) audioManager.playMusic("miniloop14");
    audioManager.setMusicVolume(32);

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
    if(e.type == SDL_KEYDOWN){
        if(e.key.keysym.sym == SDLK_F1){
            effectManager.spawn("explosion_missile", mScreenWidth / 2, mEffectiveHeight / 2);
        }
    }
}

void Game::update(float deltaTime){
    mScrollingOffset -= GLOBAL_SPEED * deltaTime;
    if(mScrollingOffset < -TextureManager::getInstance().getTexture("bg")->getWidth()){
        mScrollingOffset = 0;
    }

    projectileManager.update(deltaTime);
    explosionManager.update(deltaTime);
    playerManager.update(deltaTime);
    particleManager.update(deltaTime);
    effectManager.update(deltaTime);

    mPizza.erase(
            std::remove_if(mPizza.begin(), mPizza.end(),
                [](const ScoreCollectable& col){
                return !col.isAlive;
                }),
            mPizza.end()
            );

    for(size_t i = 0; i < mPizza.size(); i++){
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
    int offsetX = effectManager.getShakeX();
    int offsetY = effectManager.getShakeY();

    SDL_Rect viewport = {offsetX, offsetY, mScreenWidth, mScreenHeight};

    SDL_RenderSetViewport(mRenderer, &viewport);

    SDL_SetRenderDrawColor(mRenderer, 135, 206, 235, 0xFF);
    SDL_RenderClear(mRenderer);

    LTexture* bg = TextureManager::getInstance().getTexture("bg");

    bg->render(mScrollingOffset, 0);
    bg->render(mScrollingOffset + bg->getWidth(), 0);

    explosionManager.render(mRenderer);
    playerManager.render(mRenderer);
    particleManager.render(mRenderer);
    effectManager.render();

    for(size_t i = 0; i < mPizza.size(); i++){
        mPizza[i].render(mRenderer);
    }


    SDL_Rect indicatorRect;
    indicatorRect.w = mScreenWidth / mPlayerNumber;
    indicatorRect.h = 50;
    indicatorRect.y = mScreenHeight - indicatorRect.h;

    projectileManager.render(mRenderer);

    SDL_RenderSetViewport(mRenderer, NULL);


    for(size_t i = 0; i < playerManager.players.size(); i++){
        Uint8 greyIntensity = i*20 + 150;
        std::string playerNumber = "Player " + std::to_string(i + 1);
        std::string score = std::to_string(playerManager.players[i].getScore());


        LTexture scoreTexture;
        LTexture playerNumberTexture;

        scoreTexture.setRenderer(mRenderer);
        playerNumberTexture.setRenderer(mRenderer);

        playerNumberTexture.loadFromRenderedText(playerNumber, mWhite, mScoreFont);
        scoreTexture.loadFromRenderedText(score, mGreen, mScoreFont);


        indicatorRect.x = i * indicatorRect.w;
        SDL_Rect backLifeRect;
        SDL_Rect lifeRect;
        backLifeRect.x = indicatorRect.x + indicatorRect.w * 0.15f;
        backLifeRect.y = indicatorRect.y + indicatorRect.h * 0.50f;
        backLifeRect.h = indicatorRect.h * 0.25f;
        backLifeRect.w = indicatorRect.w * 0.5f;
        lifeRect = backLifeRect;
        if(playerManager.players[i].getLife() > 0){
            lifeRect.w = (float)playerManager.players[i].getLife() / (float)playerManager.players[i].getMaxLife() * (float)backLifeRect.w;
        }else{
            lifeRect.w = 0;
        }

        int timerX = backLifeRect.x + backLifeRect.w + indicatorRect.w * 0.15f;
        int timerY = indicatorRect.y + indicatorRect.h * 0.5f;
        int radius = 16;
        float progress = playerManager.players[i].getAbilityProgress();

        SDL_SetRenderDrawColor(mRenderer, greyIntensity, greyIntensity, greyIntensity, 255);
        SDL_RenderFillRect(mRenderer, &indicatorRect);
        SDL_SetRenderDrawColor(mRenderer, 255, 0, 0, 255);
        SDL_RenderFillRect(mRenderer, &backLifeRect);
        SDL_SetRenderDrawColor(mRenderer, 0, 255, 0, 255);
        SDL_RenderFillRect(mRenderer, &lifeRect);
        SDL_SetRenderDrawColor(mRenderer, 255, 255, 255, 255);
        util::drawProgressPie(mRenderer, timerX, timerY, radius, progress);

        playerManager.players[i].getSkin()->render((i + 1) * indicatorRect.w - 42, indicatorRect.y + 10);
        playerManager.players[i].getHat()->render((i + 1) * indicatorRect.w - 42, indicatorRect.y + 10);

        playerNumberTexture.render(indicatorRect.x + 3, indicatorRect.y + 3);
        scoreTexture.render(indicatorRect.x + 10, indicatorRect.y + playerNumberTexture.getHeight() + 10);
    }

    SDL_RenderPresent(mRenderer);
};

void Game::close() {
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
