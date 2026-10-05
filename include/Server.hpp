#pragma once

#include "PlayerSlot.hpp"
#include "Transport.hpp"
#include "World.hpp"

static constexpr float FIXED_DT = 1.f / 60.f;
static constexpr int MAX_STEPS_PER_FRAME = 5;

class Server{
public:
    void init(PlayerSlot* slots, int count, int screenW, int screenH);
    void setTransport(ServerTransport* transport);
    void update(float realDeltaTime);
    void setSnapshotInterval(uint32_t ticks){mSnapshotInterval = ticks;}

    const World& world() const;
    World& world();
private:
    World mWorld;
    ServerTransport* mTransport = nullptr;
    std::vector<PlayerInput> mInputs;
    uint32_t mSnapshotCounter = 0;
    float mAccumulator = 0.f;
    uint32_t mSnapshotInterval = 1;
};
