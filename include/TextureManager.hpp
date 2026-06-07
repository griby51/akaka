#pragma once

#include <unordered_map>
#include <string>
#include <memory>
#include <filesystem>
#include <vector>
#include <SDL2/SDL.h>

#include "LTexture.hpp"

class TextureManager{
public:
    static TextureManager& getInstance(){
        static TextureManager instance;
        return instance;
    }

    TextureManager(const TextureManager&) = delete;
    TextureManager& operator=(const TextureManager&) = delete;

    void init(SDL_Renderer* renderer);

    bool loadTexture(const std::string& id, const std::string& path);
    std::vector<std::string> loadDirectory(const std::string& folderPath, const std::string& prefixId);

    LTexture* getTexture(const std::string& id);
    void clean();
private:
    TextureManager() {}
    ~TextureManager() { clean(); }

    std::unordered_map<std::string, std::unique_ptr<LTexture>> mTextureMap;
    SDL_Renderer* mRenderer = nullptr;
};
