#include "Protocol.hpp"

#include <algorithm>

void writeInput(ByteWriter& w, const PlayerInput& in){
    uint8_t bits = 0;
    if(in.left) bits |= 1 << 0;
    if(in.right) bits |= 1 << 1;
    if(in.thrust) bits |= 1 << 2;
    if(in.ability) bits |= 1 << 3;

    w.u8(bits);
}

bool readInput(ByteReader& r, PlayerInput& out){
    uint8_t bits = r.u8();
    if(!r.ok()) return false;

    out.left = (bits & (1 << 0)) != 0;
    out.right = (bits & (1 << 1)) != 0;
    out.thrust = (bits & (1 << 2)) != 0;
    out.ability = (bits & (1 << 3)) != 0;

    return true;
}

static void writeEntities(ByteWriter& w, const std::vector<EntityState>& entities){
    w.u16((uint16_t)entities.size());
    for(const EntityState& e : entities){
        w.u32(e.netId);
        w.u16(e.texture);
        w.f32(e.x);
        w.f32(e.y);
        w.f32(e.angle);
        w.u16(e.w);
        w.u16(e.h);
    }
}

static bool readEntities(ByteReader& r, std::vector<EntityState>& out){
    uint16_t count = r.u16();
    if(!r.ok() || count > MAX_NET_PROJECTILES) return false;

    out.reserve(count);
    for(uint16_t i = 0; i < count; i++){
        EntityState e;
        e.netId = r.u32();
        e.texture = r.u16();
        e.x = r.f32();
        e.y = r.f32();
        e.angle = r.f32();
        e.w = r.u16();
        e.h = r.u16();

        out.push_back(e);
    }

    return r.ok();
}

static void writeEvent(ByteWriter& w, const GameEvent& e){
    w.u8((uint8_t)e.type);
    w.u16(e.id);
    w.f32(e.x);
    w.f32(e.y);
    w.f32(e.a);
    w.f32(e.b);
    w.u32(e.handle);
}

static void readEvent(ByteReader& r, GameEvent& e){
    e.type = (EventType)r.u8();
    e.id = r.u16();
    e.x = r.f32();
    e.y = r.f32();
    e.a = r.f32();
    e.b = r.f32();
    e.handle = r.u32();
}

void writeSnapshot(ByteWriter& w, const Snapshot& snap){
    w.u32(snap.tick);
    w.f32(snap.scrollingOffset);

    w.u16((uint16_t)snap.players.size());
    for(const PlayerState& p : snap.players){
        w.u8(p.index);
        w.f32(p.x);
        w.f32(p.y);
        w.f32(p.vx);
        w.f32(p.vy);
        w.i16(p.life);
        w.i32(p.score);

        w.u8(p.abilityProgress);

        uint8_t flags = 0;
        if(p.isAlive) flags |= 1 << 0;
        if(p.isControlled) flags |= 1 << 1;
        if(p.thrusting) flags |= 1 << 2;
        w.u8(flags);
    }

    writeEntities(w, snap.projectiles);
    writeEntities(w, snap.collectables);

    w.u16((uint16_t)snap.events.size());
    for(const GameEvent& e : snap.events){
        writeEvent(w, e);
    }
}

bool readSnapshot(ByteReader& r, Snapshot& out){
    out = Snapshot();
    out.tick = r.u32();
    out.scrollingOffset = r.f32();

    uint16_t playerCount = r.u16();
    if(!r.ok() || playerCount > MAX_NET_PLAYERS) return false;

    out.players.reserve(playerCount);
    for(uint16_t i = 0; i < playerCount; i++){
        PlayerState p;
        p.index = r.u8();
        p.x = r.f32();
        p.y = r.f32();
        p.vx = r.f32();
        p.vy = r.f32();
        p.life = r.i16();
        p.score = r.i32();

        p.abilityProgress = r.u8();

        uint8_t flags = r.u8();
        p.isAlive = (flags & (1 << 0)) != 0;
        p.isControlled = (flags & (1 << 1)) != 0;
        p.thrusting = (flags & (1 << 2)) != 0;

        out.players.push_back(p);
    }

    if(!readEntities(r, out.projectiles)) return false;
    if(!readEntities(r, out.collectables)) return false;

    uint16_t eventCount = r.u16();
    if(!r.ok() || eventCount > MAX_NET_EVENTS) return false;

    out.events.reserve(eventCount);
    for(uint16_t i = 0; i < eventCount; i++){
        GameEvent e;
        readEvent(r, e);
        out.events.push_back(e);
    }

    return r.ok();
}

void writeSnapshotMessage(ByteWriter& w, const Snapshot& snap){
    w.u8((uint8_t)MsgType::Snapshot);
    writeSnapshot(w, snap);
}

void writeInputMessage(ByteWriter& w, const std::vector<uint8_t>& indices, const std::vector<PlayerInput>& inputs){
    size_t count = std::min(indices.size(), inputs.size());
    if(count > MAX_NET_PLAYERS) count = MAX_NET_PLAYERS;

    w.u8((uint8_t)MsgType::Input);
    w.u8((uint8_t)count);

    for(size_t i = 0; i < count; i++){
        w.u8(indices[i]);
        writeInput(w, inputs[i]);
    }
}

bool readInputMessage(ByteReader& r, std::vector<uint8_t>& outIndices, std::vector<PlayerInput>& outInputs){
    uint8_t count = r.u8();
    if(!r.ok() || count > MAX_NET_PLAYERS) return false;

    outIndices.clear();
    outInputs.clear();

    for(uint8_t i = 0; i < count; i++){
        uint8_t index = r.u8();

        PlayerInput in;
        if(!readInput(r, in)) return false;

        outIndices.push_back(index);
        outInputs.push_back(in);
    }

    return r.ok();
}
