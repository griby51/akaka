#pragma once
#include <cstdint>
#include <string>
#include <vector>
enum class EventType { Sfx, StopSfx, Effect, Shake };


struct GameEvent{
    EventType type;
    std::string id;
    float x = 0.f,y = 0.f;
    float a = 0.f, b = 0.f;
    uint32_t handle = 0;
};

class EventQueue{
public:
    uint32_t sfx(const std::string& id);
    void stopSfx(uint32_t handle);
    void effect(const std::string& id, float x, float y, float scale);
    void shake(float intensity, float duration);

    const std::vector<GameEvent>& events() const;
    void clear();
private:
    std::vector<GameEvent> mEvents;
    uint32_t mNextHandle = 1;
};

