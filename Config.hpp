#pragma once

#include <SDL2/SDL.h>
#include <unordered_map>
#include <string>
#include <vector>
#include <cstdio>

class GameConfig{
    private:
        std::unordered_map<std::string, std::string> values;
    public:
        GameConfig(const std::string& filename = "config.ini");

        int getInt(const std::string& key, int defaultVal);
        float getFloat(const std::string& key, float defaultVal);
        bool getBool(const std::string& key, bool defaultVal);
        std::string getString(const std::string& key, const std::string& defaultVal);

        bool save();
};

extern GameConfig g_config;
