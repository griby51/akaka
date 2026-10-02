#include "Projectile.hpp"
#include "AssetIds.hpp"
#include "GameContext.hpp"
#include "TextureManager.hpp"
#include <sol/error.hpp>
#include <sol/protect.hpp>
#include <sol/protected_function_result.hpp>

namespace projectile{
    Projectile::Projectile(sol::table params, GameContext* ctx) : ctx(ctx){
        textureId = params.get_or("texture", std::string(""));
        textureAssetId = AssetIds::getInstance().id(textureId);
        LTexture* tex = TextureManager::getInstance().getTexture(textureId);
        x = params.get_or("x", 0.f);
        y = params.get_or("y", 0.f);
        w = params.get_or("width", tex ? tex->getWidth() : 0);
        h = params.get_or("height", tex ? tex->getHeight() : 0);
        vx = params.get_or("vx", 0.f);
        vy = params.get_or("vy", 0.f);
        data = sol::table(params.lua_state(), sol::create);
        isAlive = true;

        onUpdate = params["onUpdate"];
    }

    void Projectile::update(float dt){
        if(!isAlive) return;
        if(onUpdate.valid()){
            sol::protected_function_result result = onUpdate(shared_from_this(), ctx, dt);

            if(!result.valid()){
                sol::error err = result;
                printf("Projectile error : %s\n", err.what());
                kill();
                return;
            }
        }

        if(!isAlive) return;

        x += vx * dt;
        y += vy * dt;

        if(isOffScreen(5000)) kill();
    }

    bool Projectile::isOffScreen(float margin) const{
        return (w + x < -margin
                || x > *ctx->screenWidth + margin
                || h + y < -margin
                || y > *ctx->screenHeight + margin);
    }

    void Projectile::setAngle(float degrees){
        angle = degrees;
    }

    void Projectile::setVelocity(float nvx, float nvy){
        vx = nvx;
        vy = nvy;
    }

    void Projectile::setPosition(float nx, float ny){
        x = nx;
        y = ny;
    }

}
