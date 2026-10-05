#include "NetSession.hpp"

#include <cstdio>

#include "NetConfig.hpp"
#include "Protocol.hpp"

NetSession::NetSession()
    : mLoopServer(&mLink, 0),
    mLoopClient(&mLink, 0){
}

NetSession& netSession(){
    static NetSession session;
    return session;
}

bool NetSession::start(){
    if(mStarted) return true;

    const NetConfig& config = netConfig();
    mStarted = true;

    if(config.mode == NetConfig::Mode::Client){
        if(!mEnetClient.connect(config.address, config.port)){
            printf("[net] cannot reach %s:%u\n", config.address.c_str(), config.port);
            return false;
        }

        mClientOnly = true;
        return true;
    }

    if(config.mode == NetConfig::Mode::Host){
        if(mEnetServer.start(config.port, MAX_NET_PLAYERS)){
            mHosting = true;
        }else{
            printf("[net] host mode disabled, playing offline\n");
        }
    }

    return true;
}

void NetSession::stop(){
    mEnetServer.stop();
    mEnetClient.disconnect();

    mClientOnly = false;
    mHosting = false;
    mStarted = false;
}

bool NetSession::isClientOnly() const{
    return mClientOnly;
}

bool NetSession::isHosting() const{
    return mHosting;
}

ServerTransport* NetSession::loopServer(){
    return &mLoopServer;
}

ClientTransport* NetSession::loopClient(){
    return &mLoopClient;
}

EnetServer* NetSession::enetServer(){
    return mHosting ? &mEnetServer : nullptr;
}

ClientTransport* NetSession::enetClient(){
    return mClientOnly ? &mEnetClient : nullptr;
}

void NetSession::pushPendingClientMessage(std::vector<uint8_t> data){
    mPendingClientMessages.push_back(std::move(data));
}

std::vector<std::vector<uint8_t>> NetSession::takePendingClientMessages(){
    std::vector<std::vector<uint8_t>> out = std::move(mPendingClientMessages);
    mPendingClientMessages.clear();

    return out;
}

