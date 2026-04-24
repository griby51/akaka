#pragma once
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <string>
#include <sstream>
#include <memory>

#include "Config.hpp"
#include "LTexture.hpp"
#include "LTimer.hpp"
#include "Projectile.hpp"
#include "Dot.hpp"
#include "Player.hpp"
#include "Particle.hpp"
#include "PlayerSlot.hpp"
#include "CollisionSystem.hpp"
#include "Missile.hpp"
#include "ScoreCollectable.hpp"

class Game {
public:
    Game();
    ~Game();

    bool init(SDL_Renderer* renderer, SDL_Window* window, PlayerSlot* playerSlots, int joinedCount);
    bool loadMedia();
    void close();
    void start();
    void handleEvents(const SDL_Event& e);
    void handleInput();
    void update(float deltaTime);
    void render();
    bool isOver();
private:
    SDL_Window* mWindow = nullptr;
    SDL_Renderer* mRenderer = nullptr;
    SDL_Joystick* mController = nullptr;

    GameConfig mConfig;
    GameConfig mThrustParticleGameConfig;
    GameConfig mMissileConfig;

    int mScreenWidth;
    int mScreenHeight;
    int mEffectiveHeight;
    int mPlayerNumber;
    int mScoreToLaunchMissile;
    int mMissileScorePenality;
    int mPizzaTimeUntilNext;

    bool mQuit = false;
    
    static constexpr int FPS = 60;
    static constexpr int TICKS_PER_FRAME = 1000 / 60;
    static constexpr int JOYSTICK_DEAD_ZONE = 8000;
    static constexpr int THRUST_PARTICLE_NUMBER = 500;
    static constexpr int MISSILE_NUMBER = 500;
    static constexpr float GLOBAL_SPEED = 50.0f;

    std::vector<Player> mPlayers;
    ThrustParticle mThrustParticles[THRUST_PARTICLE_NUMBER];
    Missile mMissiles[MISSILE_NUMBER];
    Dot mDot;

    LTexture mSquirellTexture;
    LTexture mProjectileTexture;
    LTexture mDotTexture;
    LTexture mBGTexture;
    LTexture mScoreTexture;
    LTexture mMissileTexture;
    LTexture mCowboyTexture;
    LTexture mPizzaTexture;

    TTF_Font* mScoreFont = nullptr;

    LTimer mCapTimer;
    LTimer mDeltaTimer;
    LTimer mPizzaTimer;
    std::vector<LTimer> mParticleTimers;
    std::vector<LTimer> mMissileTimers;
    

    SDL_Rect mProjectileRect;
    float mScrollingOffset = 0;
    int mCurrentMissile = 0;
    int mCurrentThrustParticle = 0;
    int mXJoystickDir = 0;

    std::vector<LTexture*> hats;
    std::vector<LTexture*> skins;

    ParticleConfig mThrustParticleConfig;

    SDL_Color mWhite = {255, 255, 255, 255};
    SDL_Color mRed = {255, 0, 0, 255};
    SDL_Color mGreen = {0, 255, 0, 255};

    std::vector<ScoreCollectable> mPizza;

    void playerThrust(int playerIndex);
    void spawnMissile(int playerWhoSpawn);
 };
