#include "Player.hpp"

void Player::init(GameConfig* config){
    x = 0;
    y = 0;
    vx = 0;
    vy = 0;
    g_config = config;
    JETPACK_FORCE = g_config->getFloat("JETPACK_FORCE", 1000.0f);
    MAX_VX = g_config->getFloat("MAX_VX", 1000.0f);
    ACCELERATION = g_config->getFloat("ACCELERATION", 1000.0f);
    DECELERATION = g_config->getFloat("DECELERATION", 0.50f);
    GRAVITY_FORCE = g_config->getFloat("GRAVITY_FORCE", -500.0f);
    BOUNCE = g_config->getBool("BOUNCE", true);
    BOUNCE_RESTITUTION = g_config->getFloat("BOUNCE_RESTITUTION", 0.8f);
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

}

void Player::setScreenSize(int width, int height){
    SCREEN_WIDTH = width;
    SCREEN_HEIGHT = height;
}

void Player::jetpack(){
    jetpackThrust = JETPACK_FORCE;
}

void Player::render(){
    skin->render(x, y);
    if(hat != nullptr){
        hat->render(x, y);
    }
}

void Player::setSkin(LTexture* _skin){
    skin = _skin;
}

void Player::setHat(LTexture* _hat){
    hat = _hat;
}
