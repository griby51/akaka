#include "Client.hpp"

#include "ByteBuffer.hpp"
#include "NetSession.hpp"
#include "Protocol.hpp"

void Client::setTransport(ClientTransport* transport){
    mTransport = transport;
}

void Client::setOwnedPlayers(std::vector<uint8_t> indices){
    mOwned = std::move(indices);
}

void Client::sendInputs(const std::vector<PlayerInput>& inputs){
    if(!mTransport) return;

    ByteWriter w;
    writeInputMessage(w, mOwned, inputs);

    mTransport->send(w.data(), false);
}

void Client::poll(){
    if(!mTransport) return;

    std::vector<std::vector<uint8_t>> messages = netSession().takePendingClientMessages();

    for(std::vector<uint8_t>& received : mTransport->receive()){
        messages.push_back(std::move(received));
    }

    for(const std::vector<uint8_t>& message : messages){
        if(message.empty()) continue;

        ByteReader r(message.data(), message.size());

        MsgType type = (MsgType)r.u8();
        if(!r.ok()) continue;

        if(type == MsgType::Snapshot){
            Snapshot snap;
            if(readSnapshot(r, snap)){
                mSnapshot = std::move(snap);
                mHasSnapshot = true;
            }
        }else if(type == MsgType::Ownership){
            std::vector<uint8_t> owned;
            if(readOwnership(r, owned)) mOwned = std::move(owned);
        }else if(type == MsgType::Welcome){
            WelcomeData data;
            if(readWelcome(r, data)){
                mWelcome = std::move(data);
                mHasWelcome = true;
            }
        }
    }
}

const Snapshot& Client::snapshot() const{
    return mSnapshot;
}

bool Client::hasSnapshot() const{
    return mHasSnapshot;
}

bool Client::hasWelcome() const{
    return mHasWelcome;
}

const WelcomeData& Client::welcome() const{
    return mWelcome;
}

void Client::clearWelcome(){
    mHasWelcome = false;
}

size_t Client::ownedCount() const{
    return mOwned.size();
}

void Client::requestJoin(){
    if(!mTransport) return;

    ByteWriter w;
    writeJoinPlayerMessage(w);

    mTransport->send(w.data(), true);
}

