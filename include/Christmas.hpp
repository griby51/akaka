#include "Explosion.hpp"
#include "LTexture.hpp"
#include "LTimer.hpp"
#include "Player.hpp"
#include "ExplosionManager.hpp"
#include <vector>

struct ChristmasSleighConfig{
    float maxDamage = 50.f;
    float maxRadius = 10.f;
    float sleighSpeed = 100.f;
    float dropPercent = 0.1f;
    int giftMinDetonation = 500;
    int giftMaxDetonation = 10000;
    float giftMinSpeed = 3.f;
    float giftMaxSpeed = 10.f;
    int screenWidth = 666;
    int screenHeight = 666; //come later...
};

class ChristmasSleigh{
    public:
        ChristmasSleigh(std::vector<player::Player>* players, ChristmasSleighConfig config, explode::ExplosionManager* explosionManager, explode::ExplosionConfig cfg);
        bool KidnapMe(player::Player*);
        void update(float deltaTime);
        void render(SDL_Renderer* renderer);

    private:
        float maxDamage;
        float maxRadius;
        float sleighSpeed;
        float dropPercent;
        int giftMinDetonation;
        int giftMaxDetonation;
        float giftMinSpeed;
        float giftMaxSpeed;
        int screenWidth;
        int screenHeight;

};
