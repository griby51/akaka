#pragma once

#include "Transport.hpp"
#include <cstdint>
#include <enet/enet.h>
#include <unordered_map>
#include <vector>

class EnetServer : public ServerTransport{
public:
    ~EnetServer() override;

    bool start(uint16_t port, int maxClients);
    void stop();

    void sendTo(int clientId, const std::vector<uint8_t>& data, bool reliable) override;
    void broadcast(const std::vector<uint8_t>& data, bool reliable) override;
    std::vector<NetMessage> receive() override;

    std::vector<int> takeConnected();
    std::vector<int> takeDisconnected();
private:
    ENetHost* mHost = nullptr;
    std::unordered_map<int, ENetPeer*> mPeers;
    int mNextClientId = 1;
    std::vector<int> mConnected;
    std::vector<int> mDisconnected;
};
