#include "Game.hpp"
#include "Ability.hpp"
#include "LTexture.hpp"
#include "Player.hpp"
#include "ScoreCollectable.hpp"
#include "TextureManager.hpp"
#include "AnimationManager.hpp"
#include "AssetIds.hpp"
#include "InputSampler.hpp"
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
    mWorld.screenWidth = mConfig.getInt("SCREEN_WIDTH", 800);
    mWorld.screenHeight = mConfig.getInt("SCREEN_HEIGHT", 600);
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

    mWorld.init();
    mWorld.context.particleManager = &particleManager;


    SDL_RenderGetLogicalSize(mRenderer, &mWorld.screenWidth, &mWorld.screenHeight);

    mWorld.effectiveHeight = mWorld.screenHeight - 50;

    mWorld.playerManager.players.reserve(joinedCount);

    mThrustParticleConfig.load(mThrustParticleGameConfig);

    for(int i = 0; i < joinedCount; i++){
        player::PlayerConfig cfg;

        cfg.players = &mWorld.playerManager.players;
        cfg.skin = TextureManager::getInstance().getTexture(playerSlot[i].skinId);
        cfg.hat = TextureManager::getInstance().getTexture(playerSlot[i].hatId);
        cfg.skinId = playerSlot[i].skinId;
        cfg.hatId = playerSlot[i].hatId;
        cfg.audioManager = &audioManager;
        cfg.events = &mWorld.events;
        cfg.particleManager = &particleManager;

        cfg.ability = ScriptEngine::getInstance().createAbilityForHat(playerSlot[i].hatId, &mWorld.context);

        cfg.jetpackForce = mConfig.getFloat("player_jetpack_force", 700.f);
        cfg.maxVx = mConfig.getFloat("player_max_vx", 1000.f);
        cfg.acceleration = mConfig.getFloat("player_acceleration", 1000.f);
        cfg.deceleration = mConfig.getFloat("player_deceleration", 0.8f);
        cfg.maxHealth = mConfig.getInt("player_health", 100);
        cfg.bounce = mConfig.getBool("player_bounce", true);
        cfg.bounceRestitution = mConfig.getFloat("bounce_restitution", 0.4f);
        cfg.showCollider = mConfig.getBool("show_player_collider", false);
        cfg.gravityForce = mConfig.getFloat("gravity", -500.f);
        

        if(playerSlot[i].presetIndex >= 0){
            cfg.keyPreset = presets[playerSlot[i].presetIndex];
        }
        cfg.joystickId = playerSlot[i].joystickId;
        cfg.thrustParticleConfig = mThrustParticleConfig;
        cfg.screenWidth = mWorld.screenWidth;
        cfg.screenHeight = mWorld.effectiveHeight;

        PlayerInfo info;
        info.skinId = playerSlot[i].skinId;
        info.hatId = playerSlot[i].hatId;
        info.maxLife = cfg.maxHealth;
        info.colliderW = cfg.collider.w;
        info.colliderH = cfg.collider.h;
        info.showCollider = cfg.showCollider;
        mPlayerInfos.push_back(info);

        mWorld.playerManager.addPlayer(std::move(cfg));
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

    LTexture* bg = TextureManager::getInstance().getTexture("bg");
    if(bg) mWorld.backgroundWidth = bg->getWidth();

    mWorld.start();
}

void Game::handleEvents(const SDL_Event& e) {
    if (e.type == SDL_QUIT){
        mQuit = true;
        return;
    }
    if(e.type == SDL_KEYDOWN){
        if(e.key.keysym.sym == SDLK_F1){
            effectManager.spawn("explosion_missile", mWorld.screenWidth / 2, mWorld.effectiveHeight / 2);
        }
    }
}

void Game::update(float realDeltaTime){
    mAccumulator += realDeltaTime;

    int steps = 0;
    while(mAccumulator >= FIXED_DT){
        if(steps >= MAX_STEPS_PER_FRAME){
            mAccumulator = 0.f;
            break;
        }

        const Uint8* keys = SDL_GetKeyboardState(NULL);
        std::vector<PlayerInput> inputs;
        inputs.reserve(mWorld.playerManager.players.size());
        for(auto& player : mWorld.playerManager.players){
            inputs.push_back(input::sample(player.getKeyPreset(), player.getJoystickId(), keys));
        }

        mWorld.step(FIXED_DT, inputs);

        mAccumulator -= FIXED_DT;
        steps++;
    }

    particleManager.update(realDeltaTime);
    effectManager.update(realDeltaTime);

    drainEvents();
}

void Game::drainEvents(){
    for(const GameEvent& e : mWorld.events.events()){
        switch(e.type){
            case EventType::Sfx: {
                int channel = audioManager.playSFX(AssetIds::getInstance().name(e.id));
                if(channel >= 0) mSfxChannels[e.handle] = channel;
                break;
            }
            case EventType::StopSfx: {
                auto it = mSfxChannels.find(e.handle);
                if(it != mSfxChannels.end()){
                    audioManager.stopChannel(it->second);
                    mSfxChannels.erase(it);
                }
                break;
            }
            case EventType::Effect:
                effectManager.spawn(AssetIds::getInstance().name(e.id), e.x, e.y, e.a);
                break;
            case EventType::Shake:
                effectManager.triggerShake(e.a, e.b);
                break;
        }
    }

    for(auto it = mSfxChannels.begin(); it != mSfxChannels.end();){
        if(audioManager.isPlaying(it->second)){
            ++it;
        }else{
            it = mSfxChannels.erase(it);
        }
    }

    mWorld.events.clear();
}

void Game::render(){
    renderSnapshot(captureSnapshot(mWorld));
}

void Game::renderSnapshot(const Snapshot& snap){
    TextureManager& tm = TextureManager::getInstance();
    AssetIds& assets = AssetIds::getInstance();

    int offsetX = effectManager.getShakeX();
    int offsetY = effectManager.getShakeY();

    SDL_Rect viewport = {offsetX, offsetY, mWorld.screenWidth, mWorld.screenHeight};

    SDL_RenderSetViewport(mRenderer, &viewport);

    SDL_SetRenderDrawColor(mRenderer, 135, 206, 235, 0xFF);
    SDL_RenderClear(mRenderer);

    LTexture* bg = tm.getTexture("bg");

    bg->render(snap.scrollingOffset, 0);
    bg->render(snap.scrollingOffset + bg->getWidth(), 0);

    for(const PlayerState& p : snap.players){
        if(!p.isAlive) continue;
        if(p.index >= mPlayerInfos.size()) continue;

        const PlayerInfo& info = mPlayerInfos[p.index];

        LTexture* skin = tm.getTexture(info.skinId);
        LTexture* hat = tm.getTexture(info.hatId);

        if(skin) skin->render((int)p.x, (int)p.y);
        if(hat) hat->render((int)p.x, (int)p.y);

        if(info.showCollider){
            SDL_Rect collider = {(int)p.x, (int)p.y, info.colliderW, info.colliderH};
            SDL_SetRenderDrawColor(mRenderer, 255, 0, 255, 255);
            SDL_RenderDrawRect(mRenderer, &collider);
        }
    }

    particleManager.render(mRenderer);
    effectManager.render();

    for(const EntityState& e : snap.collectables){
        LTexture* texture = tm.getTexture(assets.name(e.texture));
        if(texture) texture->render((int)e.x, (int)e.y);
    }

    for(const EntityState& e : snap.projectiles){
        LTexture* texture = tm.getTexture(assets.name(e.texture));
        if(texture) texture->render((int)e.x, (int)e.y, NULL, e.angle);
    }

    SDL_RenderSetViewport(mRenderer, NULL);

    SDL_Rect indicatorRect;
    indicatorRect.w = mWorld.screenWidth / mPlayerNumber;
    indicatorRect.h = 50;
    indicatorRect.y = mWorld.screenHeight - indicatorRect.h;

    for(size_t i = 0; i < snap.players.size(); i++){
        const PlayerState& p = snap.players[i];
        if(p.index >= mPlayerInfos.size()) continue;

        const PlayerInfo& info = mPlayerInfos[p.index];

        Uint8 greyIntensity = i*20 + 150;
        std::string playerNumber = "Player " + std::to_string(i + 1);
        std::string score = std::to_string(p.score);

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
        if(p.life > 0 && info.maxLife > 0){
            lifeRect.w = (float)p.life / (float)info.maxLife * (float)backLifeRect.w;
        }else{
            lifeRect.w = 0;
        }

        int timerX = backLifeRect.x + backLifeRect.w + indicatorRect.w * 0.15f;
        int timerY = indicatorRect.y + indicatorRect.h * 0.5f;
        int radius = 16;
        float progress = (float)p.abilityProgress / 255.f;

        SDL_SetRenderDrawColor(mRenderer, greyIntensity, greyIntensity, greyIntensity, 255);
        SDL_RenderFillRect(mRenderer, &indicatorRect);
        SDL_SetRenderDrawColor(mRenderer, 255, 0, 0, 255);
        SDL_RenderFillRect(mRenderer, &backLifeRect);
        SDL_SetRenderDrawColor(mRenderer, 0, 255, 0, 255);
        SDL_RenderFillRect(mRenderer, &lifeRect);
        SDL_SetRenderDrawColor(mRenderer, 255, 255, 255, 255);
        util::drawProgressPie(mRenderer, timerX, timerY, radius, progress);

        LTexture* skin = tm.getTexture(info.skinId);
        LTexture* hat = tm.getTexture(info.hatId);
        if(skin) skin->render((i + 1) * indicatorRect.w - 42, indicatorRect.y + 10);
        if(hat) hat->render((i + 1) * indicatorRect.w - 42, indicatorRect.y + 10);

        playerNumberTexture.render(indicatorRect.x + 3, indicatorRect.y + 3);
        scoreTexture.render(indicatorRect.x + 10, indicatorRect.y + playerNumberTexture.getHeight() + 10);
    }

    SDL_RenderPresent(mRenderer);
}

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
