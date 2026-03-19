#include "Particle.hpp"

void ParticleConfig::load(GameConfig& config){
    int count = config.getInt("particle_color_count", 0);

    keyframes.clear();
    for(int i = 0; i < count; i++){
        std::string key = "particle_color_" + std::to_string(i);
        std::string val = config.getString(key, "255,255,255,255,500");
        ColorKeyframe kf;
        sscanf(val.c_str(), "%hhu,%hhu,%hhu,%hhu,%d",
                &kf.r, &kf.g, &kf.b, &kf.a, &kf.time);
        keyframes.push_back(kf);
    }
    growRate = config.getFloat("growRate", 0.0f);
    friction = config.getFloat("friction", 0.0f);
    riseForce = config.getFloat("riseForce", 0.0f);
    vxSpread = config.getInt("vxSpread", 0);
    vyMin = config.getInt("vyMin", 0);
    vyMax = config.getInt("vyMax", 0);
    sizeMin = config.getInt("sizeMin", 10);
    sizeMax = config.getInt("sizeMax" ,10);

    setColorFrameList();
}


void ParticleConfig::setColorFrameList(){
    int totalTime = keyframes.back().time;

    for(int t = 0; t <= totalTime; t++){
        for(int i = 0; i < (int)keyframes.size() - 1; i++){
            ColorKeyframe& a = keyframes[i];
            ColorKeyframe& b = keyframes[i + 1];

            if(t >= a.time && t <= b.time){
                float pct = (float)(t - a.time) / (b.time - a.time);
                SDL_Color color;
                color.r = a.r + pct * (b.r - a.r);
                color.g = a.g + pct * (b.g - a.g);
                color.b = a.b + pct * (b.b - a.b);
                color.a = a.a + pct * (b.a - a.a);
                colorFrameList.push_back(color);
                break;
            }
        }
    }

    int tableSize = colorFrameList.size();
    printf("Table size: %i\n", tableSize);
}

int ParticleConfig::getMaxTime(){
    return keyframes.back().time;
}

SDL_Color ParticleConfig::getCurrentColor(int currentTime){
    SDL_Color color = colorFrameList[currentTime];
    return color;
}

void Particle::init(ParticleConfig* particleConfig){
    particleRect.x = 10000;
    particleRect.y = 10000;
    particleRect.w = 10;
    particleRect.h = 10;
    config = particleConfig;
    maxLifeTime = static_cast<float>(config->getMaxTime());
    isAlive = false;
}

void Particle::setPos(int posX, int posY){
    particleRect.x = posX;
    particleRect.y = posY;
}

void Particle::reset(){
    lifeTime = 0.0f;
    isAlive = true;
     vx = ((rand() % (config->vxSpread * 2)) - config->vxSpread) / 100.0f;
     vy = (rand() % (config->vyMax - config->vyMin) + config->vyMin) / 100.0f;
     size = rand() % (config->sizeMax - config->sizeMin) + config->sizeMin;
     particleRect.w = size;
     particleRect.h = size;
}

void Particle::update(float deltaTime){
   if(!isAlive) return;
   if(!((lifeTime+=(deltaTime*1000.0f)) <= maxLifeTime)){
       isAlive = false;
       return;
   };
   intCurrentTime = static_cast<int>(lifeTime);

   particleRect.x += vx;
   particleRect.y += vy;
   vy -= config->riseForce;
   vx *= config->friction;
   size += config->growRate;
   particleRect.w = (int)size;
   particleRect.h = (int)size;
}

int Particle::getX(){
    return particleRect.x;
}

int Particle::getY(){
    return particleRect.y;
}

void Particle::render(SDL_Renderer* renderer){
    if(!isAlive){
        return;
    };
    SDL_Color color = config->getCurrentColor(intCurrentTime);
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_RenderFillRect(renderer, &particleRect);
}


