#pragma once

#include "Transport.hpp"
#include <cstdint>
#include <vector>
class LoopbackLink{
public:
    void pushToServer(int clientId, const std::vector<uint8_t>& data);
    void pushToClient(const std::vector<uint8_t>& data);

    std::vector<NetMessage> takeServerMessages();
    std::vector<std::vector<uint8_t>> takeClientMessages();
private:
    std::vector<NetMessage> mToServer;
    std::vector<std::vector<uint8_t>> mToClient;
};

class LoopbackClient : public ClientTransport{
public:
    LoopbackClient(LoopbackLink* link, int clientId);

    void send(const std::vector<uint8_t>& data, bool reliable) override;
    std::vector<std::vector<uint8_t>> receive() override;
    bool isConnected() const override;
private:
    LoopbackLink* mLink = nullptr;
    int mClientId = 0;
};

class LoopbackServer : public ServerTransport{
public:
    LoopbackServer(LoopbackLink* link, int clientId);

    void sendTo(int clientId, const std::vector<uint8_t>& data, bool reliable) override;
    void broadcast(const std::vector<uint8_t>& data, bool reliable) override;
    std::vector<NetMessage> receive() override;
private:
    LoopbackLink* mLink = nullptr;
    int mClientId = 0;
};
