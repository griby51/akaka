#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <enet/enet.h>
#include "Transport.hpp"
class EnetClient : public ClientTransport{
public:
    ~EnetClient() override;

    bool connect(const std::string& adress, uint16_t port, uint32_t timeoutMs = 3000);
    bool disconnect();

    void send(const std::vector<uint8_t>& data, bool reliable) override;
    std::vector<std::vector<uint8_t>> receive() override;
    bool isConnected() const override;
private:
    ENetHost* mHost = nullptr;
    ENetPeer* mPeer = nullptr;
    bool mConnected = false;
};
