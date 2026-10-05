#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
class AssetIds{
public:
    AssetIds(const AssetIds&) = delete;
    AssetIds& operator = (const AssetIds&) = delete;
    static AssetIds& getInstance();

    uint16_t id(const std::string& name);
    const std::string& name(uint16_t id) const;
    uint16_t count() const;
    void loadTable(const std::vector<std::string>& names);
    const std::vector<std::string>& names() const;
private:
    AssetIds();
    std::unordered_map<std::string, uint16_t> mIds;
    std::vector<std::string> mNames;
};
