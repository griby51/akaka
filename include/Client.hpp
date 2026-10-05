#pragma once

#include "PlayerInput.hpp"
#include "Protocol.hpp"
#include "Snapshot.hpp"
#include "Transport.hpp"
class Client{
public:
    void setTransport(ClientTransport* transport);
    void sendInputs(const std::vector<PlayerInput>& inputs);
    void poll();

    bool hasWelcome() const;
    const WelcomeData& welcome() const;
    void clearWelcome();

    const Snapshot& snapshot() const;
    bool hasSnapshot() const;
    void setOwnedPlayers(std::vector<uint8_t> indices);
    size_t ownedCount() const;
    void requestJoin();
private:
    ClientTransport* mTransport = nullptr;
    Snapshot mSnapshot;
    WelcomeData mWelcome;
    bool mHasWelcome = false;
    bool mHasSnapshot = false;
    std::vector<uint8_t> mOwned;
};
