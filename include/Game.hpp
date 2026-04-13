#pragma once
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <string>
#include <sstream>

#include "Config.hpp"
#include "LTexture.hpp"
#include "LTimer.hpp"
#include "Projectile.hpp"
#include "Dot.hpp"
#include "Player.hpp"
#include "Particle.hpp"
#include "CollisionSystem.hpp"
#include "Missile.hpp"

class Game {
public:
    Game();
    ~Game();

    bool init();
    bool loadMedia();
    void run();
    void close();
private:
    SDL_Window* mWindow = nullptr;
    SDL_Renderer* mRenderer = nullptr;
    SDL_Joystick* mController = nullptr;

    GameConfig mConfig;
    GameConfig mThrustParticleGameConfig;
    GameConfig mMissileConfig;

    int mScreenWidth;
    int mScreenHeight;
    int mPlayerNumber;
    int mScoreToLaunchMissile;
    int mMissileScorePenality;
    
    static constexpr int FPS = 60;
    static constexpr int TICKS_PER_FRAME = 1000 / 60;
    static constexpr int JOYSTICK_DEAD_ZONE = 8000;
    static constexpr int THRUST_PARTICLE_NUMBER = 500;
    static constexpr int PROJECTILE_NUMBER = 20;
    static constexpr int MISSILE_NUMBER = 50;
    static constexpr float GLOBAL_SPEED = 5.0f;

    std::vector<Player> mPlayers;
    ThrustParticle mThrustParticles[THRUST_PARTICLE_NUMBER];
    Projectile mProjectiles[PROJECTILE_NUMBER];
    Missile mMissiles[MISSILE_NUMBER];
    Dot mDot;

    LTexture mSquirellTexture;
    LTexture mProjectileTexture;
    LTexture mDotTexture;
    LTexture mBGTexture;
    LTexture mScoreTexture;
    LTexture mMissileTexture;
    LTexture mCowboyTexture;

    TTF_Font* mScoreFont = nullptr;

    LTimer mCapTimer;
    LTimer mProjectileTimer;
    LTimer mCollisionTimer;
    LTimer mScoreTimer;
    LTimer mDeltaTimer;
    std::vector<LTimer> mParticleTimers;
    std::vector<LTimer> mMissileTimers;

    SDL_Rect mProjectileRect;
    int mScrollingOffset = 0;
    int mCurrentProj = 0;
    int mCurrentMissile = 0;
    int mCurrentThrustParticle = 0;
    int mRandomBaseProjectileSpawnTicks = 0;
    int mRandomProjectilePos = 0;
    int mXJoystickDir = 0;

    ParticleConfig mThrustParticleConfig;

    SDL_Color mWhite = {255, 255, 255, 255};

    void handleEvents(bool &quit);
    void update(float deltaTime);
    void render();
    void playerThrust(int playerIndex);
    void spawnMissile(int playerWhoSpawn);

 };
