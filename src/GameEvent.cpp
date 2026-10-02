#include "GameEvent.hpp"
#include "AssetIds.hpp"

uint32_t EventQueue::sfx(const std::string& id){
    GameEvent e;
    e.type = EventType::Sfx;
    e.id = AssetIds::getInstance().id(id);
    e.handle = mNextHandle++;

    mEvents.push_back(e);
    return e.handle;
}

void EventQueue::stopSfx(uint32_t handle){
    GameEvent e;
    e.type = EventType::StopSfx;
    e.handle = handle;

    mEvents.push_back(e);
}

void EventQueue::effect(const std::string& id, float x, float y, float scale){
    GameEvent e;
    e.type = EventType::Effect;
    e.id = AssetIds::getInstance().id(id);
    e.x = x;
    e.y = y;
    e.a = scale;

    mEvents.push_back(e);
}

void EventQueue::shake(float intensity, float duration){
    GameEvent e;
    e.type = EventType::Shake;
    e.a = intensity;
    e.b = duration;

    mEvents.push_back(e);
}

const std::vector<GameEvent>& EventQueue::events() const { return mEvents; }

void EventQueue::clear(){ mEvents.clear(); }
