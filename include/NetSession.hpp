#pragma once

#include "EnetClient.hpp"
#include "EnetServer.hpp"
#include "LoopbackTransport.hpp"

class NetSession{
public:
    NetSession();

    bool start();
    void stop();

    bool isClientOnly() const;
    bool isHosting() const;

    void pushPendingClientMessage(std::vector<uint8_t> data);
    std::vector<std::vector<uint8_t>> takePendingClientMessages();

    ServerTransport* loopServer();
    ClientTransport* loopClient();
    EnetServer* enetServer();
    ClientTransport* enetClient();

private:
    LoopbackLink mLink;
    LoopbackServer mLoopServer;
    LoopbackClient mLoopClient;
    EnetServer mEnetServer;
    EnetClient mEnetClient;

    bool mClientOnly = false;
    bool mHosting = false;
    bool mStarted = false;
    std::vector<std::vector<uint8_t>> mPendingClientMessages;
};

NetSession& netSession();
