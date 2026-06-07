#include "TextureManager.hpp"
#include "LTexture.hpp"
#include <filesystem>
#include <vector>

void TextureManager::init(SDL_Renderer* renderer){
    mRenderer = renderer;
}

bool TextureManager::loadTexture(const std::string& id, const std::string& path){
    if(mTextureMap.find(id) != mTextureMap.end()) return true;

    auto newTexture = std::make_unique<LTexture>();
    newTexture->setRenderer(mRenderer);

    if(!newTexture->loadFromeFile(path)){
        printf("Error TextureManager : can't load : %s\n", path.c_str());
        return false;
    }

    mTextureMap[id] = std::move(newTexture);
    return true;
}

std::vector<std::string> TextureManager::loadDirectory(const std::string& folderPath, const std::string& prefixId){
    std::vector<std::string> loadedIds;

    for(const auto& entry : std::filesystem::directory_iterator(folderPath)){
        if(entry.path().extension() == ".png"){
            std::string id = prefixId + entry.path().stem().string();
            if(loadTexture(id, entry.path().string())){
                loadedIds.push_back(id);
            }
        }
    }

    return loadedIds;
}

LTexture* TextureManager::getTexture(const std::string& id){
    auto it = mTextureMap.find(id);
    if(it != mTextureMap.end()){
        return it->second.get();
    }
    return nullptr;
}

void TextureManager::clean(){
    mTextureMap.clear();
}
