#pragma once
#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include <string>
#include <unordered_map>
#include <cstdio>

class AudioManager{
private:
    std::unordered_map<std::string, Mix_Music*> musicMap;
    std::unordered_map<std::string, Mix_Chunk*> sfxMap;
public:
    AudioManager();
    ~AudioManager();

    bool init();
    void clean();

    bool loadMusic(const std::string& id, const std::string& path);
    bool loadSFX(const std::string& id, const std::string& path);

    void playMusic(const std::string& id, int loops=-1);
    int playSFX(const std::string& id, int loops=0, int channel = -1);

    void stopMusic();
    void setMusicVolume(int volume);

    void stopChannel(int channel);
};

