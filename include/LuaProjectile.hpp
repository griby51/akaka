#pragma once

#include "Projectile.hpp"
#include <sol/sol.hpp>
#include <memory>
#include <string>
#include <tuple>

struct GameContext;

namespace projectile{
    class LuaProjectile : public Projectile, public std::enable_shared_from_this<LuaProjectile>{
    public:
        LuaProjectile(sol::table params, GameContext* ctx);

        void update(float dt) override;
        void render(SDL_Renderer* renderer) override;
        bool isOffScreen(float margin = 0.f) const;
        bool isValid() const{return isAlive;}
        void setVelocity(float nvx, float nvy);
        void setAngle(float degrees);
        void setPosition(float nx, float ny);
        std::tuple<float, float> getPosition() const{return {x, y}; }
        std::tuple<float, float> getVelocity() const{return {vx, vy}; }
        std::tuple<int, int> getSize() const{return {w, h}; }
        sol::table getData() const{return data; }

    private:
        std::string textureId;
        float angle = 0.f;
        int w = 0;
        int h = 0;

        GameContext* ctx = nullptr;
        sol::protected_function onUpdate;
        sol::table data;

        SDL_Rect getCollider() const {return { (int)x, (int)y, w, h }; }


    };
}
