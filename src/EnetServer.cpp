#include "EnetServer.hpp"

#include <cstdio>

EnetServer::~EnetServer(){
    stop();
}

bool EnetServer::start(uint16_t port, int maxClients){
    if(mHost) stop();

    ENetAddress addr;
    addr.host = ENET_HOST_ANY;
    addr.port = port;

    mHost = enet_host_create(&addr, maxClients, 2, 0, 0);
    if(!mHost){
        printf("cannot listen on port %u\n", port);
        return false;
    }

    printf("listening on port %u\n", port);
    return true;
}

void EnetServer::stop(){
    if(!mHost) return;

    for(auto& entry : mPeers){
        if(entry.second) enet_peer_disconnect_now(entry.second, 0);
    }
    mPeers.clear();

    enet_host_destroy(mHost);
    mHost = nullptr;
}

void EnetServer::sendTo(int clientId, const std::vector<uint8_t>& data, bool reliable){
    if(!mHost || data.empty()) return;

    auto it = mPeers.find(clientId);
    if(it == mPeers.end() || !it->second) return;

    ENetPacket* packet = enet_packet_create(data.data(), data.size(), reliable ? ENET_PACKET_FLAG_RELIABLE : 0);
    if(!packet) return;

    if(enet_peer_send(it->second, reliable ? 0 : 1, packet) < 0){
        enet_packet_destroy(packet);
    }
}

void EnetServer::broadcast(const std::vector<uint8_t>& data, bool reliable){
    if(!mHost || data.empty() || mPeers.empty()) return;

    ENetPacket* packet = enet_packet_create(data.data(), data.size(), reliable ? ENET_PACKET_FLAG_RELIABLE : 0);
    if(!packet) return;

    enet_host_broadcast(mHost, reliable ? 0 : 1, packet);
}

std::vector<NetMessage> EnetServer::receive(){
    std::vector<NetMessage> out;
    if(!mHost) return out;

    ENetEvent event;

    while(enet_host_service(mHost, &event, 0) > 0){
        switch(event.type){
            case ENET_EVENT_TYPE_CONNECT: {
                int id = mNextClientId++;
                event.peer->data = (void*)(intptr_t)id;
                mPeers[id] = event.peer;
                mConnected.push_back(id);

                printf("client %d connected\n", id);
                break;
            }

            case ENET_EVENT_TYPE_RECEIVE: {
                NetMessage message;
                message.clientId = (int)(intptr_t)event.peer->data;
                message.data.assign(event.packet->data, event.packet->data + event.packet->dataLength);

                out.push_back(std::move(message));
                enet_packet_destroy(event.packet);
                break;
            }

            case ENET_EVENT_TYPE_DISCONNECT: {
                int id = (int)(intptr_t)event.peer->data;
                mPeers.erase(id);
                mDisconnected.push_back(id);
                event.peer->data = nullptr;

                printf("client %d disconnected\n", id);
                break;
            }

            default:
                break;
        }
    }

    return out;
}

std::vector<int> EnetServer::takeConnected(){
    std::vector<int> out = std::move(mConnected);
    mConnected.clear();

    return out;
}

std::vector<int> EnetServer::takeDisconnected(){
    std::vector<int> out = std::move(mDisconnected);
    mDisconnected.clear();

    return out;
}
