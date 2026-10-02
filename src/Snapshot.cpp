#include "Snapshot.hpp"

#include "Player.hpp"
#include "Projectile.hpp"
#include "World.hpp"

Snapshot captureSnapshot(const World& world){
    Snapshot snap;
    snap.tick = world.tick;
    snap.scrollingOffset = world.scrollingOffset;

    const std::vector<player::Player>& players = world.playerManager.players;
    snap.players.reserve(players.size());

    for(size_t i = 0; i < players.size(); i++){
        const player::Player& p = players[i];

        PlayerState state;
        state.index = (uint8_t)i;
        state.x = p.getX();
        state.y = p.getY();
        state.vx = p.getVx();
        state.vy = p.getVy();
        state.life = (int16_t)p.getLife();
        state.score = (int32_t)p.getScore();
        state.isAlive = p.isAlive;
        state.isControlled = p.isControlled;
        state.thrusting = p.isThrusting();
        state.abilityProgress = (uint8_t)(p.getAbilityProgress() * 255.f);

        snap.players.push_back(state);
    }

    const std::vector<std::shared_ptr<projectile::Projectile>>& projectiles = world.projectileManager.all();
    snap.projectiles.reserve(projectiles.size());

    for(const std::shared_ptr<projectile::Projectile>& p : projectiles){
        if(!p || p->isDead()) continue;

        auto [px, py] = p->getPosition();
        auto [pw, ph] = p->getSize();

        EntityState state;
        state.netId = p->getNetId();
        state.texture = p->getTextureAssetId();
        state.x = px;
        state.y = py;
        state.angle = p->getAngle();
        state.w = (uint16_t)pw;
        state.h = (uint16_t)ph;

        snap.projectiles.push_back(state);
    }

    snap.collectables.reserve(world.pizzas.size());
    for(const ScoreCollectable& c : world.pizzas){
        if(!c.isAlive) continue;

        EntityState state;
        state.texture = c.textureAssetId;
        state.x = c.x;
        state.y = c.y;
        state.w = (uint16_t)c.collider.w;
        state.h = (uint16_t)c.collider.h;

        snap.collectables.push_back(state);
    }

    snap.events = world.events.events();

    return snap;
}
