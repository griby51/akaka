#include "Client.hpp"

#include "ByteBuffer.hpp"
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

    for(const std::vector<uint8_t>& message : mTransport->receive()){
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
        }
    }
}

const Snapshot& Client::snapshot() const{
    return mSnapshot;
}

bool Client::hasSnapshot() const{
    return mHasSnapshot;
}
