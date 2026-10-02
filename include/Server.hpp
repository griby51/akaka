#pragma once

#include "PlayerSlot.hpp"
#include "Transport.hpp"
#include "World.hpp"
class Server{
public:
    void init(PlayerSlot* slots, int count, int screenW, int screenH);
    void setTransport(ServerTransport* transport);
    void update(float realDeltaTime);

    const World& world() const;
private:
    World mWorld;
    ServerTransport* mTransport = nullptr;
    std::vector<PlayerInput> mInputs;
    uint32_t mSnapshotCounter = 0;
};
