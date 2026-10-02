#pragma once

#include "PlayerInput.hpp"
#include "Snapshot.hpp"
#include "Transport.hpp"
class Client{
public:
    void setTransport(ClientTransport* transport);
    void sendInputs(const std::vector<PlayerInput>& inputs);
    void poll();

    const Snapshot& snapshot() const;
    bool hasSnapshot() const;
    void setOwnedPlayers(std::vector<uint8_t> indices);
private:
    ClientTransport* mTransport = nullptr;
    Snapshot mSnapshot;
    bool mHasSnapshot = false;
    std::vector<uint8_t> mOwned;
};
