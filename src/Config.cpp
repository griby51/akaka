#include "Config.hpp"

GameConfig::GameConfig(const std::string& filename){
    SDL_RWops* file = SDL_RWFromFile(filename.c_str(), "r");
    if(!file){
        printf("Cannot load config file : %s, SDL_Error : %s\n", filename.c_str(), SDL_GetError());
        return;
    }

    Sint64 size = SDL_RWsize(file);
    char* buffer = new char[size + 1];
    SDL_RWread(file, buffer, 1, size);
    buffer[size] = '\0';

    std::string content(buffer);
    delete[] buffer;
    SDL_RWclose(file);

    size_t pos = 0;
    std::string line;

    while((pos = content.find('\n')) != std::string::npos){
        line = content.substr(0, pos);
        content.erase(0, pos+1);

        if(line.empty() || line[0] == '#' || line[0] == ';'){
            continue;
        }

        size_t eq = line.find('=');
        if(eq != std::string::npos){
            std::string key = line.substr(0, eq);
            std::string val = line.substr(eq + 1);

            key.erase(0, key.find_first_not_of(" \t"));
            key.erase(key.find_last_not_of(" \t") + 1);
            val.erase(0, val.find_first_not_of(" \t"));
            val.erase(val.find_last_not_of(" \t") + 1);

            values[key] = val;
        }
    }
}

int GameConfig::getInt(const std::string& key, int defaultVal){
    auto it = values.find(key);
    return it != values.end() ? std::stoi(it->second) : defaultVal;
}

float GameConfig::getFloat(const std::string& key, float defaultVal){
    auto it = values.find(key);
    return it != values.end() ? std::stof(it->second) : defaultVal;
}

bool GameConfig::getBool(const std::string& key, bool defaultVal){
    auto it = values.find(key);
    if( it!=values.end()){
        std::string v = it->second;
        for(char& c : v) c = tolower(c);
        return v == "true" || v == "1" || v == "yes" || v == "on";
    }
    return defaultVal;
}

std::string GameConfig::getString(const std::string& key, const std::string& defaultVal){
    auto it = values.find(key);
    return it != values.end() ? it->second : defaultVal;
}

bool GameConfig::save(){
    std::string data;
    printf("Saving %zu keys\n", values.size());
    for(const auto& p : values){
        printf(" %s = %s\n", p.first.c_str(), p.second.c_str());
        data += p.first + "=" + p.second + "\n";
    }

    SDL_RWops* file = SDL_RWFromFile("config.ini", "w");
    if(!file) return false;

    SDL_RWwrite(file, data.c_str(), 1, data.size());
    SDL_RWclose(file);
    return true;
}

