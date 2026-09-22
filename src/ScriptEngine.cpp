#include "ScriptEngine.hpp"
#include "Ability.hpp"
#include "GameContext.hpp"
#include "LuaAbility.hpp"
#include "Player.hpp"
#include "TextureManager.hpp"
#include "AudioManager.hpp"
#include "EffectManager.hpp"

#include <memory>
#include <sol/forward.hpp>
#include <sol/optional_implementation.hpp>
#include <sol/property.hpp>
#include <sol/sol.hpp>
#include <cstdio>
#include <sol/types.hpp>
#include <vector>
#include <algorithm>
#include <cmath>

static std::vector<player::Player*> findPlayersInRadius(GameContext& ctx, float x, float y, float radius, const std::vector<player::Player*>& ignore){
    std::vector<player::Player*> result;
    if(!ctx.players) return result;

    float squareRadius = radius*radius;

    for(auto& p : *ctx.players){
        if(std::find(ignore.begin(), ignore.end(), &p) != ignore.end()) continue;
        if(!p.isAlive) continue;

        float cx = p.collider.x + p.collider.w / 2.f;
        float cy = p.collider.y + p.collider.h / 2.f;
        
        float dx = cx - x;
        float dy = cy - y;

        float dSquare = (dx * dx + dy * dy);

        if(dSquare <= squareRadius){
            result.push_back(&p);
        }
    }

    return result;
}

void ScriptEngine::init(){
    lua = std::make_unique<sol::state>();

    lua->open_libraries(
            sol::lib::base,
            sol::lib::math,
            sol::lib::string,
            sol::lib::table
    );

    lua->create_named_table("_abilities");
    lua->create_named_table("_hats");

    registerBindings();
}

ScriptEngine::ScriptEngine() = default;
ScriptEngine::~ScriptEngine() = default;

void ScriptEngine::registerBindings(){
    lua->set_function("loadTexture", [](const std::string& id, const std::string& path){
            return TextureManager::getInstance().loadTexture(id, path);
            });
    lua->set_function("registerHat", [this](sol::table def){
            sol::optional<std::string> id = def["id"];
            if(!id){
                printf("No id, hat ignored\n");
                return;
            }
            sol::optional<std::string> texture = def["texture"];
            if(!texture){
                printf("registerHat %s : no texture, ignored\n", id->c_str());
                return;
            }
            if(!TextureManager::getInstance().loadTexture(*id, *texture)){
                printf("registerHat %s : texture cant be loaded : %s\n", id->c_str(), texture->c_str());
                return;
            }

            sol::table hats = (*lua)["_hats"];
            hats.add(def);

            printf("[lua] hat saved : %s\n", id->c_str());
            });
    lua->new_usertype<player::Player>("Player",
            sol::no_constructor,
            "setVelocity", &player::Player::setVelocity,
            "teleport", &player::Player::teleportTo,
            "applyKnockBack", &player::Player::applyKnockBack,
            "getLife", &player::Player::getLife,
            "getMaxLife", &player::Player::getMaxLife,
            "getScore", &player::Player::getScore,
            "addScore", &player::Player::updateScore,
            "isAlive", sol::readonly(&player::Player::isAlive),
            "damage", [](player::Player& self, int amount){self.updateLife(-amount); },
            "heal", [](player::Player& self, int amount){self.updateLife(amount); },
            "getPosition", [](player::Player& self){
                return std::make_tuple((float)self.collider.x, (float)self.collider.y);
            },
            "getCenter", [](player::Player& self){
                return std::make_tuple(self.collider.x + self.collider.w / 2.0f,
                        self.collider.y + self.collider.h / 2.0f);
            },
            "getSize", [](player::Player& self){
                return std::make_tuple(self.collider.w, self.collider.h);
            }
            );


    lua->new_usertype<GameContext>("GameContext",
            sol::no_constructor,
            "playersInRadius",[](GameContext& self, float x, float y, float radius, sol::optional<std::vector<player::Player*>> ignore){
                return sol::as_table(findPlayersInRadius(self, x, y, radius, ignore.value_or(std::vector<player::Player*>{})));
            },
            "explode", [](GameContext& self, sol::table params){
                std::vector<player::Player*> hits;

                sol::optional<float> x = params["x"];
                sol::optional<float> y = params["y"];
                sol::optional<float> radius = params["radius"];

                if(!x || !y || !radius){
                    printf("[lua] explode : x, y and radius are required\n");
                    return sol::as_table(hits);
                }

                float damage = params.get_or("damage", 0.f);
                float force = params.get_or("force", 0.f);
                auto ignore = params.get<sol::optional<std::vector<player::Player*>>>("ignore");

                hits = findPlayersInRadius(self, *x, *y, *radius, ignore.value_or(std::vector<player::Player*>{}));

                float squareRadius = *radius * *radius;

                for(player::Player* p : hits){
                    float cibleCx = p->collider.x + p->collider.w / 2.f;
                    float cibleCy = p->collider.y + p->collider.h / 2.f;

                    float dx = cibleCx - *x;
                    float dy = cibleCy - *y;

                    float distSq = dx*dx + dy*dy;

                    float factor = 1.f - (distSq) / squareRadius;

                    float dist = std::sqrt(distSq);
                    float dirX = 0.f;
                    float dirY = -1.f;

                    if(dist > 0.f){
                        dirX = dx / dist;
                        dirY = dy / dist;
                    }

                    p->updateLife(-static_cast<int>(damage*factor));

                    float forceX = dirX * force * factor;
                    float forceY = dirY * force * factor;

                    p->applyKnockBack(forceX, forceY);
                }

                return sol::as_table(hits);
            },
            "playSFX", [](GameContext& self, const std::string& id){
                if(self.audioManager) self.audioManager->playSFX(id);
            },
            "shakeScreen", [](GameContext& self, float intensity, float duration){
                if(self.effectManager) self.effectManager->triggerShake(intensity, duration);
            },
            "spawnEffect", [](GameContext& self, const std::string& animId, float x, float y, sol::optional<float> scale){
                if(self.effectManager){
                    self.effectManager->spawn(animId, x, y, scale.value_or(1.f));
                }
            });
    lua->set_function("registerAbility", [this](sol::table def){
            sol::optional<std::string> id = def["id"];
            if(!id){
                printf("registerAbility : no id, ability ignored\n");
                return;
            }
            sol::optional<sol::protected_function> onUse = def["onUse"];
            if(!onUse){
                printf("regiterAbility : %s : onUse not found, ability ignored\n", id->c_str());
                return;
            }

            sol::table store = (*lua)["_abilities"];

            if(store[*id].valid()){
                printf("registerAbility : %s was already declared, replaced\n", id->c_str());
            }

            store[*id] = def;
            printf("[lua] ability saved : %s\n", id->c_str());

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

std::unique_ptr<Ability> ScriptEngine::createAbility(const std::string& id, GameContext* ctx){
    if(!lua) return nullptr;

    sol::table store = (*lua)["_abilities"];
    sol::optional<sol::table> def = store[id];

    if(!def){
        printf("createAbility : unknown ability : %s\n", id.c_str());
        return nullptr;
    }

    sol::table instance = lua->create_table();
    instance[sol::metatable_key] = lua->create_table_with("__index", *def);

    return std::make_unique<LuaAbility>(instance, ctx);
}

std::unique_ptr<Ability> ScriptEngine::createAbilityForHat(const std::string& hatId, GameContext* ctx){
    if(!lua) return nullptr;

    sol::table hats = (*lua)["_hats"];
    for(size_t i = 1; i <= hats.size(); i++){
        sol::table hat = hats[i];

        if(hat.get_or("id", std::string()) != hatId) continue;

        std::string abilityId = hat.get_or("ability", std::string());
        if(abilityId.empty()) return nullptr;

        return createAbility(abilityId, ctx);
    }

    return nullptr;
}
