#pragma once
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>

#include <cstdint>
#include <unordered_map>

#include "Config.hpp"
#include "LTexture.hpp"
#include "LTimer.hpp"
#include "ParticleManager.hpp"
#include "Particle.hpp"
#include "PlayerInfo.hpp"
#include "PlayerSlot.hpp"
#include "Snapshot.hpp"
#include "EffectManager.hpp"
#include "Client.hpp"
#include "LoopbackTransport.hpp"
#include "Server.hpp"
#include "AudioManager.hpp"

class Game {
public:
    Game();
    ~Game();

    bool init(SDL_Renderer* renderer, SDL_Window* window, PlayerSlot* playerSlots, int joinedCount);
    bool loadMedia();
    void close();
    void start();
    void handleEvents(const SDL_Event& e);
    void update(float realDeltaTime);
    void render();
    bool isOver();
private:
    SDL_Window* mWindow = nullptr;
    SDL_Renderer* mRenderer = nullptr;
    SDL_Joystick* mController = nullptr;

    GameConfig mConfig;
    GameConfig mThrustParticleGameConfig;

    int mPlayerNumber;

    bool mQuit = false;
    


    AudioManager audioManager;
    EffectManager effectManager;
    LoopbackLink mLink;
    LoopbackServer mLoopServer;
    LoopbackClient mLoopClient;
    Server mServer;
    Client mClient;
    std::vector<PlayerInfo> mPlayerInfos;
    std::vector<uint8_t> mOwnedPlayers;
    ParticleManager particleManager;


    TTF_Font* mScoreFont = nullptr;

    


    ParticleConfig mThrustParticleConfig;

    SDL_Color mWhite = {255, 255, 255, 255};
    SDL_Color mRed = {255, 0, 0, 255};
    SDL_Color mGreen = {0, 255, 0, 255};


    void drainEvents(const std::vector<GameEvent>& events);
    void renderSnapshot(const Snapshot& snap);

    std::unordered_map<uint32_t, int> mSfxChannels;

 };
