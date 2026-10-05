#include "EnetClient.hpp"

#include <cstdio>
#include <cstdint>
#include <enet/enet.h>
#include <enet/protocol.h>

EnetClient::~EnetClient(){
    disconnect();
}

bool EnetClient::connect(const std::string& adress, uint16_t port, uint32_t timeoutMs){
    if(mHost) disconnect();

    mHost = enet_host_create(NULL, 1, 2, 0, 0);
    if(!mHost){
        printf("enet_host_create failed\n");
        return false;
    }

    ENetAddress addr;
    if(enet_address_set_host(&addr, adress.c_str()) != 0){
        printf("adress not found : %s\n", adress.c_str());
        disconnect();
        return false;
    }

    addr.port = port;

    mPeer = enet_host_connect(mHost, &addr, 2, 0);
    if(!mPeer){
        printf("no peer available\n");
        disconnect();
        return false;
    }

    ENetEvent event;
    if(enet_host_service(mHost, &event, timeoutMs) > 0 && event.type == ENET_EVENT_TYPE_CONNECT){
        mConnected = true;
        printf("connected to %s:%u\n", adress.c_str(), port);
        return true;
    }

    printf("connection to %s:%u failed : timeout\n", adress.c_str(), port);
    enet_peer_reset(mPeer);
    mPeer = nullptr;
    disconnect();
    return false;
}

bool EnetClient::disconnect(){
    if(mHost && mPeer && mConnected){
        enet_peer_disconnect(mPeer, 0);

        ENetEvent event;

        while(enet_host_service(mHost, &event, 100) > 0){
            if(event.type == ENET_EVENT_TYPE_RECEIVE){
                enet_packet_destroy(event.packet);
            }else if(event.type == ENET_EVENT_TYPE_DISCONNECT){
                break;
            }
        }

        enet_peer_reset(mPeer);
    }

    mPeer = nullptr;
    mConnected = false;

    if(mHost){
        enet_host_destroy(mHost);
        mHost = nullptr;
    }

    return true;
}

void EnetClient::send(const std::vector<uint8_t>& data, bool reliable){
    if(!mPeer || !mConnected || data.empty()) return;

    ENetPacket* packet = enet_packet_create(data.data(), data.size(), reliable ? ENET_PACKET_FLAG_RELIABLE : 0);
    if(!packet) return;

    if(enet_peer_send(mPeer, reliable ? 0 : 1, packet) < 0){
        enet_packet_destroy(packet);
    }
}

std::vector<std::vector<uint8_t>> EnetClient::receive(){
    std::vector<std::vector<uint8_t>> out;
    if(!mHost) return out;

    ENetEvent event;

    while(enet_host_service(mHost, &event, 0) > 0){
        switch(event.type){
            case ENET_EVENT_TYPE_CONNECT:
                mConnected = true;
                break;

            case ENET_EVENT_TYPE_RECEIVE:
                out.emplace_back(event.packet->data, event.packet->data + event.packet->dataLength);
                enet_packet_destroy(event.packet);
                break;

            case ENET_EVENT_TYPE_DISCONNECT:
                printf("disconnected from host\n");
                mConnected = false;
                mPeer = nullptr;
                break;

            default:
                break;
        }
    }

    return out;
}

bool EnetClient::isConnected() const{
    return mConnected;
}
