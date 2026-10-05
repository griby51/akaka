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
#include "KeyPreset.hpp"
#include "PlayerInfo.hpp"
#include "PlayerSlot.hpp"
#include "Snapshot.hpp"
#include "EffectManager.hpp"
#include "Client.hpp"
#include "NetSession.hpp"
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
    bool mIsClientOnly = false;
    bool mAssetsReady = false;
    uint32_t mLastSeenTick = 0;
    uint32_t mSnapshotsThisSecond = 0;
    LTimer mStatTimer;
    


    AudioManager audioManager;
    EffectManager effectManager;
    struct LocalBinding{
        KeyPreset preset;
        int joystickId = -1;
    };

    Server mServer;
    Client mClient;
    std::vector<PlayerInfo> mPlayerInfos;
    std::vector<uint8_t> mOwnedPlayers;
    std::vector<int> mClientIds;
    std::vector<LocalBinding> mBindings;
    std::vector<std::string> mSkinIds;
    std::vector<std::string> mHatIds;
    ParticleManager particleManager;


    TTF_Font* mScoreFont = nullptr;

    


    ParticleConfig mThrustParticleConfig;

    SDL_Color mWhite = {255, 255, 255, 255};
    SDL_Color mRed = {255, 0, 0, 255};
    SDL_Color mGreen = {0, 255, 0, 255};


    void drainEvents(const std::vector<GameEvent>& events);
    void renderSnapshot(const Snapshot& snap);
    int addPlayer(const PlayerSlot& slot);
    void rebuildWelcome();
    void sendOwnership(int clientId);
    void handleJoinRequests();

    std::unordered_map<uint32_t, int> mSfxChannels;

 };
