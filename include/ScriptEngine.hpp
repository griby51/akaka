#pragma once

#include <memory>
#include <string>
#include <sol/forward.hpp>

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

private:
    ScriptEngine();
    ~ScriptEngine();

    void registerBindings();
    std::unique_ptr<sol::state> lua;
};
