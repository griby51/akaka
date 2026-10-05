#pragma once

#include "PlayerSlot.hpp"
#include "Transport.hpp"
#include "World.hpp"

#include <unordered_map>
#include <utility>

static constexpr float FIXED_DT = 1.f / 60.f;
static constexpr int MAX_STEPS_PER_FRAME = 5;

class Server{
public:
    void init(PlayerSlot* slots, int count, int screenW, int screenH);
    void addTransport(ServerTransport* transport);
    void update(float realDeltaTime);
    void setSnapshotInterval(uint32_t ticks){mSnapshotInterval = ticks;}
    void setWelcomePayload(std::vector<uint8_t> payload);
    void setOwnership(int clientId, const std::vector<uint8_t>& indices);
    void addOwnership(int clientId, uint8_t index);
    const std::vector<uint8_t>& ownership(int clientId) const;

    std::vector<int> takeJoinRequests();
    void sendTo(int clientId, const std::vector<uint8_t>& data, bool reliable);
    void broadcast(const std::vector<uint8_t>& data, bool reliable);

    const World& world() const;
    World& world();
private:
    bool owns(int clientId, uint8_t index) const;

    World mWorld;
    std::vector<uint8_t> mWelcome;
    std::unordered_map<int, std::vector<uint8_t>> mOwnership;
    std::vector<int> mJoinRequests;
    std::vector<ServerTransport*> mTransports;
    std::vector<PlayerInput> mInputs;
    uint32_t mSnapshotCounter = 0;
    float mAccumulator = 0.f;
    uint32_t mSnapshotInterval = 1;
};
