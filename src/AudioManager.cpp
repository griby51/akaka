#include "AudioManager.hpp"
#include <SDL2/SDL_mixer.h>

AudioManager::AudioManager(){}

AudioManager::~AudioManager(){
    clean();
}

bool AudioManager::init(){
    if(SDL_InitSubSystem(SDL_INIT_AUDIO) < 0){
        printf("SDL_Init audio err : %s\n", SDL_GetError());
        return false;
    }

    if(Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0){
        printf("Mix_OpenAudio error: %s\n", Mix_GetError());
        return false;
    }

    Mix_AllocateChannels(64);

    return true;
}

bool AudioManager::loadMusic(const std::string& id, const std::string& path){
    if(musicMap.find(id) != musicMap.end()){
        return true;
    }

    Mix_Music* music = Mix_LoadMUS(path.c_str());
    if(!music){
        printf("Error loading music : %s, Error : %s\n", path.c_str(), Mix_GetError());
        return false;
    }

    musicMap[id] = music;
    return true;
};

bool AudioManager::loadSFX(const std::string& id, const std::string& path){
    if(sfxMap.find(id) != sfxMap.end()){
        return true;
    }

    Mix_Chunk* sfx = Mix_LoadWAV(path.c_str());
    if(!sfx){
        printf("Error loading sfx : %s, Error : %s\n", path.c_str(), Mix_GetError());
        return false;
    }

    sfxMap[id] = sfx;
    return true;
}

void AudioManager::playMusic(const std::string& id, int loops){
    if(musicMap.find(id) != musicMap.end()){
        Mix_PlayMusic(musicMap[id], loops);
    }else{
        printf("Music %s not found\n", id.c_str());
    }
}

int AudioManager::playSFX(const std::string& id, int loops, int channel){
    if(sfxMap.find(id) != sfxMap.end()){
        return Mix_PlayChannel(channel, sfxMap[id], loops);
    }else{
        printf("SFX %s not found\n", id.c_str());
        return -1;
    }
}

void AudioManager::stopMusic(){
    Mix_HaltMusic();
}

void AudioManager::setMusicVolume(int volume){
    Mix_VolumeMusic(volume);
}

void AudioManager::clean(){
    for(auto const& [id, music] : musicMap){
        Mix_FreeMusic(music);
    }

    musicMap.clear();

    for(auto const& [id, sfx] : sfxMap){
        Mix_FreeChunk(sfx);
    }

    sfxMap.clear();

    Mix_CloseAudio();
}

void AudioManager::stopChannel(int channel){
    Mix_HaltChannel(-1);
}
