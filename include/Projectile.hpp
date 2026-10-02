#pragma once

#include <SDL2/SDL.h>
#include <sol/sol.hpp>
#include <cstdint>
#include <memory>
#include <string>
#include <tuple>

struct GameContext;

namespace projectile{
    class Projectile : public std::enable_shared_from_this<Projectile>{
    public:
        Projectile(sol::table params, GameContext* ctx);

        void update(float dt);

        bool isDead() const {return !isAlive; }
        void kill(){ isAlive = false; }

        uint32_t getNetId() const {return netId; }
        void setNetId(uint32_t id){netId = id; }

        bool isOffScreen(float margin = 0.f) const;
        bool isValid() const{return isAlive;}
        void setVelocity(float nvx, float nvy);
        void setAngle(float degrees);
        void setPosition(float nx, float ny);
        std::tuple<float, float> getPosition() const{return {x, y}; }
        std::tuple<float, float> getVelocity() const{return {vx, vy}; }
        std::tuple<int, int> getSize() const{return {w, h}; }
        sol::table getData() const{return data; }

        uint16_t getTextureAssetId() const {return textureAssetId; }
        float getAngle() const {return angle; }
        SDL_Rect getCollider() const {return { (int)x, (int)y, w, h }; }

    private:
        float x = 0.f, y = 0.f;
        float vx = 0.f, vy = 0.f;
        bool isAlive = true;
        uint32_t netId = 0;

        std::string textureId;
        uint16_t textureAssetId = 0;
        float angle = 0.f;
        int w = 0;
        int h = 0;

        GameContext* ctx = nullptr;
        sol::protected_function onUpdate;
        sol::table data;
    };
}
