#pragma once

#include <cstdint>
#include <vector>

struct NetMessage{
    int clientId = -1;
    std::vector<uint8_t> data;
};

class ClientTransport{
public:
    virtual ~ClientTransport() = default;

    virtual void send(const std::vector<uint8_t>& data, bool reliable) = 0;
    virtual std::vector<std::vector<uint8_t>> receive() = 0;
    virtual bool isConnected() const = 0;
};

class ServerTransport{
public:
    virtual ~ServerTransport() = default;

    virtual void sendTo(int clientId, const std::vector<uint8_t>& data, bool reliable) = 0;
    virtual void broadcast(const std::vector<uint8_t>& data, bool reliable) = 0;
    virtual std::vector<NetMessage> receive() = 0;
};
