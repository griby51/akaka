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
}

SDL_Color ParticleConfig::getCurrentColor(int currentTime){
    SDL_Color color = colorFrameList[currentTime];
    return color;
}
