#pragma once

#include <memory>
#include <string>
#include <sol/forward.hpp>

class Ability;
struct GameContext;

class ScriptEngine{
public:
    static ScriptEngine& getInstance(){
        static ScriptEngine instance;
        return instance;
    }

    ScriptEngine(const ScriptEngine&) = delete;
    ScriptEngine& operator=(const ScriptEngine&) = delete;

    void init();
    bool runFile(const std::string& path);

    std::unique_ptr<Ability> createAbility(const std::string& id, GameContext* ctx);
    std::unique_ptr<Ability> createAbilityForHat(const std::string& hatId, GameContext* ctx);

private:
    ScriptEngine();
    ~ScriptEngine();

    void registerBindings();
    std::unique_ptr<sol::state> lua;
};
