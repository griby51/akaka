#include "AssetIds.hpp"
#include <cstdint>

AssetIds::AssetIds(){ mNames.push_back(""); }

uint16_t AssetIds::id(const std::string& name){
    auto it = mIds.find(name);
    if(it != mIds.end()) return it->second;
    
    uint16_t newId = (uint16_t)mNames.size();
    mNames.push_back(name);
    mIds[name] = newId;

    return newId;
}

const std::string& AssetIds::name(uint16_t id) const{
    static const std::string empty;
    if(id >= mNames.size()) return empty;
    return mNames[id];
}

uint16_t AssetIds::count() const{ return (uint16_t)mNames.size(); }

AssetIds& AssetIds::getInstance(){
    static AssetIds instance;
    return instance;
}

void AssetIds::loadTable(const std::vector<std::string>& names){
    mNames = names;
    if(mNames.empty()) mNames.push_back("");

    mIds.clear();
    for(size_t i = 1; i < mNames.size(); i++){
        mIds[mNames[i]] = (uint16_t)i;
    }
}

const std::vector<std::string>& AssetIds::names() const{
    return mNames;
}

