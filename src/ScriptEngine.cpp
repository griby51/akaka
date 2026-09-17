#include "ScriptEngine.hpp"
#include "GameContext.hpp"
#include "Player.hpp"
#include "TextureManager.hpp"

#include <memory>
#include <sol/sol.hpp>
#include <cstdio>

void ScriptEngine::init(){
    lua = std::make_unique<sol::state>();

    lua->open_libraries(
            sol::lib::base,
            sol::lib::math,
            sol::lib::string,
            sol::lib::table
    );

    registerBindings();
}

ScriptEngine::ScriptEngine() = default;
ScriptEngine::~ScriptEngine() = default;

void ScriptEngine::registerBindings(){
    lua->set_function("loadTexture", [](const std::string& id, const std::string& path){
            return TextureManager::getInstance().loadTexture(id, path);
            });
    lua->new_usertype<player::Player>("Player",
            sol::no_constructor,
            "setVelocity", &player::Player::setVelocity
            );
    lua->new_usertype<GameContext>("GameContext",
            sol::no_constructor,
            "playersInRadius",[](GameContext& self, float x, float y, float radius, sol::optional<player::Player&> exclude){
                std::vector<player::Player*> result;
                if(!self.players) return sol::as_table(result);

                float squareRadius = radius*radius;

                for(auto& p : *self.players){
                    
                    if(!self.players->at(i).isAlive) return;

                    float cx = p.collider.x + self.players->at(i).collider.w / 2.f;
                    float cy = self.players->at(i).collider.y + self.players->at(i).collider.h / 2.f;
                    
                    float dx = cx - x;
                    float dy = cy - y;

                    float dSquare = (dx * dx + dy * dy);

                    if(dSquare <= squareRadius){
                        result.push_back(&self.players->at(i));
                    }
                }

                return sol::as_table(result);
            });
}

bool ScriptEngine::runFile(const std::string& path){
    if(!lua){
        printf("Error, init was not called, ignored script : %s\n", path.c_str());
        return false;
    }
    
    sol::protected_function_result result = lua->safe_script_file(path, sol::script_pass_on_error);

    if(!result.valid()){
        sol::error err = result;
        printf("Error Script Engine : %s : %s\n", path.c_str(), err.what());
        return false;
    }

    return true;
}
