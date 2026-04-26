#include "Player.hpp"
#include "KeyPreset.hpp"

void Player::init(GameConfig* config, int _index){
    index = _index;
    life = 100;
    x = 0;
    y = 0;
    vx = 0;
    vy = 0;
    isAlive = true;
    thrustParticlesTimer.start();
    currentThrustParticle = 0;
    g_config = config;
    JETPACK_FORCE = g_config->getFloat("JETPACK_FORCE", 1000.0f);
    MAX_VX = g_config->getFloat("MAX_VX", 1000.0f);
    ACCELERATION = g_config->getFloat("ACCELERATION", 1000.0f);
    DECELERATION = g_config->getFloat("DECELERATION", 0.50f);
    GRAVITY_FORCE = g_config->getFloat("GRAVITY_FORCE", -500.0f);
    BOUNCE = g_config->getBool("BOUNCE", true);
    BOUNCE_RESTITUTION = g_config->getFloat("BOUNCE_RESTITUTION", 0.8f);
    scoreToLaunchMissile = g_config->getFloat("scoreToLaunchMissile", 200);
    missileTimer.start();
}

void Player::setPos(float posX, float posY){
    x = posX;
    y = posY;
}

void Player::move(int direction){
    dir = direction;
}

void Player::setCollider(int colX, int colY, int colW, int colH){
    distanceBetweenColliderXAnd0 = colX;
    distanceBetweenColliderYAnd0 = colY;
    colX += x;
    colY += y;
    collider.x = colX;
    collider.y = colY;
    collider.w = colW;
    collider.h = colH;
}

int Player::getX(){
    return x;
}

int Player::getY(){
    return y;
}

void Player::updateScore(int toAdd){
    score += toAdd;
}

void Player::drawCollider(SDL_Renderer* renderer, SDL_Color* color){

    SDL_SetRenderDrawColor(renderer, color->r, color->g, color->b, color->a);
    SDL_RenderDrawRect(renderer, &collider);
}

int Player::getScore(){
    return score;
}

void Player::update(float deltaTime){
    if(!isAlive) return;

    vx = (vx + (ACCELERATION * deltaTime * dir)) * (1 - ((1 - DECELERATION) * deltaTime));

    if(vx > MAX_VX) vx = MAX_VX;
    if (vx < -MAX_VX) vx = -MAX_VX;

    vy -= jetpackThrust*deltaTime;
    vy = vy - GRAVITY_FORCE * deltaTime;

    x += vx * deltaTime;
    y += vy * deltaTime;

    if (x < 0){ 
        x = 0;
        vx = 0;
    };
    if( x > SCREEN_WIDTH - collider.w){
        x = SCREEN_WIDTH - collider.w;
        vx = 0;
    }
    if (y < 0){ 
        y = 0;
        vy = 0;
    }
    if (y > SCREEN_HEIGHT - collider.h){
        y = SCREEN_HEIGHT - collider.h;
        vy = 0;
    };

    jetpackThrust = 0.0f;

    dir = 0;

    collider.x = x + distanceBetweenColliderXAnd0;
    collider.y = y + distanceBetweenColliderYAnd0;

    for (int i = 0; i < 500; i++){
        thrustParticles[i].update(deltaTime);
    }

    if (life <= 0){
        isAlive = false;
        printf("Player dead\n");
    }
}

void Player::setScreenSize(int width, int height){
    SCREEN_WIDTH = width;
    SCREEN_HEIGHT = height;
}

void Player::jetpack(){
    jetpackThrust = JETPACK_FORCE;

    if(thrustParticlesTimer.getTicks() >= 20){
        thrustParticlesTimer.start();
        thrustParticles[currentThrustParticle].reset();
        thrustParticles[currentThrustParticle].setPos(x + 5, y + 25);
        currentThrustParticle = (currentThrustParticle + 1) % 500;
    }
}

void Player::render(SDL_Renderer* renderer){
    if (!isAlive) return;

    skin->render(x, y);
    if(hat != nullptr){
        hat->render(x, y);
    }
    for(int i = 0; i < 500; i++){
        thrustParticles[i].render(renderer);
    }
}

void Player::setSkin(LTexture* _skin){
    skin = _skin;
}

void Player::setHat(LTexture* _hat){
    hat = _hat;
}

void Player::setKeyPreset(KeyPreset preset){
    mPreset = preset;
}

void Player::handleInput(const Uint8* keys){
    if(keys[mPreset.left]){
        move(-1);
    }
    if(keys[mPreset.right]){
        move(1);
    }
    if(keys[mPreset.thrust]){
        jetpack();
    }
    if(keys[mPreset.missile]){
        spawnMissile();
    }
}

void Player::setParticleConfig(GameConfig config){
    thrustParticleConfig.load(config);
    for(int i = 0; i < 500; i++){
        thrustParticles[i].init(&thrustParticleConfig);
    }
}

void Player::setMissileTable(Missile* _missiles, int tableSize, int* _currentMissile){
    missiles = _missiles;
    currentMissile = _currentMissile;
    missileTableSize = tableSize;
}

void Player::setPlayerTable(Player* _players, int tableSize){
    players = _players;
    playerTableSize = tableSize;
}

void Player::spawnMissile(){
    int target = index;

    if(playerTableSize <= 1) return;
    if(missileTimer.getTicks() <= 300) return;
    if(score < scoreToLaunchMissile) return;

    score-=scoreToLaunchMissile;
    missileTimer.start();

    while(target == index){
        target = rand() % playerTableSize;
    }


    *currentMissile = (*currentMissile + 1) % missileTableSize;
    missiles[*currentMissile].reset();
    missiles[*currentMissile].setPos(SCREEN_WIDTH + 50, SCREEN_HEIGHT / 2);
    missiles[*currentMissile].isAlive = true;
    missiles[*currentMissile].setTarget(&players[target].collider);
}

int Player::getLife(){
    return life;
}

void Player::updateLife(int toAdd){
    life+=toAdd;
}

void Player::setJoystickId(int id){
    mJoystickId = id;
}

void Player::handleJoystickInput(SDL_Joystick* joystick){
    if(!joystick) return;

    Sint16 axisX = SDL_JoystickGetAxis(joystick, 0);
    if(axisX < -DEAD_ZONE) move(-1);
    else if(axisX > DEAD_ZONE) move(1);

    if(SDL_JoystickGetButton(joystick, 0)) jetpack();

    if(SDL_JoystickGetButton(joystick, 1)) spawnMissile();
}

int Player::getJoystickId(){
    return mJoystickId;
}
