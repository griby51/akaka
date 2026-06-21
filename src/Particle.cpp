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
    printf("Color frame list size : %i\n", colorFrameList.size());
}


void ParticleConfig::setColorFrameList(){
    int totalTime = keyframes.back().time;

    for(int t = 0; t <= totalTime; t++){
        for(int i = 0; i < (int)keyframes.size() - 1; i++){
            ColorKeyframe& a = keyframes[i];
            ColorKeyframe& b = keyframes[i + 1];

            if(t >= a.time && t < b.time){
                float pct = (float)(t - a.time) / (b.time - a.time);
                SDL_Color color;
                color.r = a.r + pct * (b.r - a.r);
                color.g = a.g + pct * (b.g - a.g);
                color.b = a.b + pct * (b.b - a.b);
                color.a = a.a + pct * (b.a - a.a);
                colorFrameList.push_back(color);
    //printf("Current Time : %i Color : %i %i %i %i\n",t, color.r, color.g, color.b, color.a);
                break;
            }
        }
    }

    int tableSize = colorFrameList.size();
    printf("Table size: %i\n", tableSize);
}

int ParticleConfig::getMaxTime() const{
    return keyframes.back().time - 1;
}

SDL_Color ParticleConfig::getCurrentColor(int currentTime){
    SDL_Color color = colorFrameList[currentTime];
    return color;
}

void Particle::setPos(int posX, int posY){
    particleRect.x = posX;
    particleRect.y = posY;
    fx = posX;
    fy = posY;
}

int Particle::getX(){
    return particleRect.x;
}

int Particle::getY(){
    return particleRect.y;
}

void ThrustParticle::init(const ParticleConfig& particleConfig){
    particleRect.x = 10000;
    particleRect.y = 10000;
    particleRect.w = 10;
    particleRect.h = 10;

    this->friction = particleConfig.friction;
    this->growRate = particleConfig.growRate;
    this->riseForce = particleConfig.riseForce;
    this->vyMin = particleConfig.vyMin;
    this->vyMax = particleConfig.vyMax;

    this->maxLifeTime = static_cast<float>(particleConfig.getMaxTime());
    this->isAlive = true;
    this->lifeTime = 0;
    this->intCurrentTime = 0;

    if(!(particleConfig.vxSpread == 0)){
        this->vx = ((rand() % (particleConfig.vxSpread * 2)) - particleConfig.vxSpread) / 100.f;
    }

    if(!(particleConfig.vyMax == particleConfig.vyMin)){
        this->vy = (rand() % (particleConfig.vyMax - particleConfig.vyMin) + particleConfig.vyMin) / 100.f;
    }else{
        this->vy = particleConfig.vyMax;
    }

    if(!(particleConfig.sizeMax == particleConfig.sizeMin)){
        size = rand() % (particleConfig.sizeMax - particleConfig.sizeMin) + particleConfig.sizeMin;
    }else{
        size = particleConfig.sizeMax;
    }

    particleRect.w = size;
    particleRect.h = size;

    this->config = particleConfig;
}

void ThrustParticle::update(float deltaTime){
    if(!isAlive) return;
    if(!((lifeTime+=(deltaTime*1000.0f)) <= maxLifeTime)){
        isAlive = false;
        //intCurrentTime = 0;
        return;
    };
    intCurrentTime = static_cast<int>(lifeTime);

    fx+=vx;
    fy+=vy;

    particleRect.x = fx;
    particleRect.y = fy;
    vy -= riseForce;
    vx *= friction;

    size += growRate;
    particleRect.w = (int)size;
    particleRect.h = (int)size;
}

void ThrustParticle::render(SDL_Renderer* renderer){
    if(!isAlive){
        return;
    };
    SDL_Color color = config.getCurrentColor(intCurrentTime);
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_RenderFillRect(renderer, &particleRect);
}
