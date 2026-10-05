#include "LoopbackTransport.hpp"
#include "Transport.hpp"

void LoopbackLink::pushToServer(int clientId, const std::vector<uint8_t>& data){
    mToServer.push_back({clientId, data});
}

void LoopbackLink::pushToClient(const std::vector<uint8_t>& data){
    mToClient.push_back(data);
}

std::vector<NetMessage> LoopbackLink::takeServerMessages(){
    std::vector<NetMessage> out = std::move(mToServer);
    mToServer.clear();

    return out;
}

std::vector<std::vector<uint8_t>> LoopbackLink::takeClientMessages(){
    std::vector<std::vector<uint8_t>> out = std::move(mToClient);
    mToClient.clear();
    return out;
}

LoopbackClient::LoopbackClient(LoopbackLink* link, int clientId)
    : mLink(link), mClientId(clientId){}

void LoopbackClient::send(const std::vector<uint8_t>& data, bool){
    if(!mLink) return;
    mLink->pushToServer(mClientId, data);
}

std::vector<std::vector<uint8_t>> LoopbackClient::receive(){
    if(!mLink) return {};

    return mLink->takeClientMessages();
}

bool LoopbackClient::isConnected() const{ return mLink != nullptr; }

LoopbackServer::LoopbackServer(LoopbackLink* link, int clientId)
    : mLink(link), mClientId(clientId) {}

void LoopbackServer::sendTo(int clientId, const std::vector<uint8_t>& data, bool){
    if(!mLink) return;
    if(clientId != mClientId) return;

    mLink->pushToClient(data);
}

void LoopbackServer::broadcast(const std::vector<uint8_t>& data, bool){
    if(!mLink) return;

    mLink->pushToClient(data);
}

std::vector<NetMessage> LoopbackServer::receive(){
    if(!mLink) return {};

    return mLink->takeServerMessages();
}

std::vector<int> LoopbackServer::takeConnected(){
    if(mReported) return {};

    mReported = true;
    return { mClientId };
}

std::vector<int> LoopbackServer::takeDisconnected(){
    return {};
}

