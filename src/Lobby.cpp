#include "Lobby.hpp"

#include <cstdio>

#include "ByteBuffer.hpp"
#include "NetConfig.hpp"
#include "NetSession.hpp"
#include "Protocol.hpp"

void Lobby::start(){
    NetSession& session = netSession();
    if(!session.start()) return;

    mHosting = !session.isClientOnly();

    if(mHosting){
        mServerTransports.push_back(session.loopServer());
        if(session.isHosting()) mServerTransports.push_back(session.enetServer());

        mClientTransport = session.loopClient();
    }else{
        mClientTransport = session.enetClient();
    }
}

bool Lobby::isHosting() const{
    return mHosting;
}

const std::vector<LobbySlot>& Lobby::slots() const{
    return mSlots;
}

std::vector<uint8_t> Lobby::takeAssignedSlots(){
    std::vector<uint8_t> out = std::move(mAssigned);
    mAssigned.clear();

    return out;
}

bool Lobby::matchStarted() const{
    return mStarted;
}

void Lobby::clearMatchStarted(){
    mStarted = false;
}

void Lobby::broadcastState(){
    if(!mHosting) return;

    ByteWriter w;
    writeLobbyStateMessage(w, mSlots);

    for(ServerTransport* transport : mServerTransports){
        transport->broadcast(w.data(), true);
    }
}

void Lobby::requestJoin(){
    if(!mClientTransport) return;

    ByteWriter w;
    writeJoinPlayerMessage(w);

    mClientTransport->send(w.data(), true);
}

void Lobby::requestUpdate(uint8_t slotIndex, uint8_t skinIndex, uint8_t hatIndex, bool ready){
    if(!mClientTransport) return;

    ByteWriter w;
    writeSlotUpdateMessage(w, slotIndex, skinIndex, hatIndex, ready);

    mClientTransport->send(w.data(), true);
}

void Lobby::requestStart(){
    if(!mHosting) return;

    ByteWriter w;
    writeStartMatchMessage(w, mSlots);

    for(ServerTransport* transport : mServerTransports){
        transport->broadcast(w.data(), true);
    }

    mStarted = true;

    for(LobbySlot& slot : mSlots){
        slot.ready = false;
    }
}

void Lobby::handleHostMessage(int clientId, const std::vector<uint8_t>& data){
    if(data.empty()) return;

    ByteReader r(data.data(), data.size());

    MsgType type = (MsgType)r.u8();
    if(!r.ok()) return;

    if(type == MsgType::JoinPlayer){
        if(!readJoinPlayer(r)) return;
        if(mSlots.size() >= MAX_NET_PLAYERS){
            printf("[lobby] slot refused : %u players max\n", MAX_NET_PLAYERS);
            return;
        }

        LobbySlot slot;
        slot.ownerClientId = (uint8_t)clientId;

        uint8_t index = (uint8_t)mSlots.size();
        mSlots.push_back(slot);

        ByteWriter w;
        writeSlotAssignedMessage(w, index);

        for(ServerTransport* transport : mServerTransports){
            transport->sendTo(clientId, w.data(), true);
        }

        printf("[lobby] client %d took slot %u\n", clientId, index);
        broadcastState();
    }else if(type == MsgType::SlotUpdate){
        uint8_t slotIndex = 0;
        uint8_t skinIndex = 0;
        uint8_t hatIndex = 0;
        bool ready = false;

        if(!readSlotUpdate(r, slotIndex, skinIndex, hatIndex, ready)) return;
        if(slotIndex >= mSlots.size()) return;
        if(mSlots[slotIndex].ownerClientId != (uint8_t)clientId){
            printf("[lobby] update rejected : client %d does not own slot %u\n", clientId, slotIndex);
            return;
        }

        mSlots[slotIndex].skinIndex = skinIndex;
        mSlots[slotIndex].hatIndex = hatIndex;
        mSlots[slotIndex].ready = ready;

        broadcastState();
    }
}

void Lobby::handleClientMessage(const std::vector<uint8_t>& data){
    if(data.empty()) return;

    ByteReader r(data.data(), data.size());

    MsgType type = (MsgType)r.u8();
    if(!r.ok()) return;

    if(type == MsgType::LobbyState){
        std::vector<LobbySlot> slots;
        if(readLobbySlots(r, slots)) mSlots = std::move(slots);
    }else if(type == MsgType::StartMatch){
        std::vector<LobbySlot> slots;
        if(readLobbySlots(r, slots)){
            mSlots = std::move(slots);
            mStarted = true;
        }
    }else if(type == MsgType::SlotAssigned){
        uint8_t slotIndex = 0;
        if(readSlotAssigned(r, slotIndex)) mAssigned.push_back(slotIndex);
    }else{
        netSession().pushPendingClientMessage(data);
    }
}

void Lobby::poll(){
    for(ServerTransport* transport : mServerTransports){
        for(int clientId : transport->takeConnected()){
            printf("[lobby] client %d connected\n", clientId);
        }

        for(int clientId : transport->takeDisconnected()){
            printf("[lobby] client %d left\n", clientId);
        }

        for(const NetMessage& message : transport->receive()){
            handleHostMessage(message.clientId, message.data);
        }
    }

    if(mClientTransport){
        for(const std::vector<uint8_t>& data : mClientTransport->receive()){
            handleClientMessage(data);
        }
    }
}
