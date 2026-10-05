#pragma once

#include <cstdint>
#include <vector>

#include "Transport.hpp"

struct LobbySlot{
    uint8_t ownerClientId = 0;
    uint8_t skinIndex = 0;
    uint8_t hatIndex = 0;
    bool ready = false;
};

class Lobby{
public:
    void start();
    void poll();

    bool isHosting() const;

    const std::vector<LobbySlot>& slots() const;
    std::vector<uint8_t> takeAssignedSlots();

    void requestJoin();
    void requestUpdate(uint8_t slotIndex, uint8_t skinIndex, uint8_t hatIndex, bool ready);
    void requestStart();

    bool matchStarted() const;
    void clearMatchStarted();

private:
    void broadcastState();
    void handleHostMessage(int clientId, const std::vector<uint8_t>& data);
    void handleClientMessage(const std::vector<uint8_t>& data);

    std::vector<ServerTransport*> mServerTransports;
    ClientTransport* mClientTransport = nullptr;

    bool mHosting = false;
    bool mStarted = false;

    std::vector<LobbySlot> mSlots;
    std::vector<uint8_t> mAssigned;
};
