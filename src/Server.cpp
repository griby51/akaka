#include "Server.hpp"
#include "ByteBuffer.hpp"
#include "Snapshot.hpp"
#include "Transport.hpp"
#include "World.hpp"
#include "Protocol.hpp"

#include <algorithm>
#include <cstdio>
#include <cstdint>

const World& Server::world() const{ return mWorld; }
World& Server::world(){ return mWorld; }

void Server::addTransport(ServerTransport* transport){
    if(!transport) return;

    mTransports.push_back(transport);
}

void Server::setWelcomePayload(std::vector<uint8_t> payload){
    mWelcome = std::move(payload);
}

void Server::setOwnership(int clientId, const std::vector<uint8_t>& indices){
    mOwnership[clientId] = indices;
}

void Server::addOwnership(int clientId, uint8_t index){
    mOwnership[clientId].push_back(index);
}

const std::vector<uint8_t>& Server::ownership(int clientId) const{
    static const std::vector<uint8_t> empty;

    auto it = mOwnership.find(clientId);
    if(it == mOwnership.end()) return empty;

    return it->second;
}

bool Server::owns(int clientId, uint8_t index) const{
    const std::vector<uint8_t>& owned = ownership(clientId);

    return std::find(owned.begin(), owned.end(), index) != owned.end();
}

std::vector<int> Server::takeJoinRequests(){
    std::vector<int> out = std::move(mJoinRequests);
    mJoinRequests.clear();

    return out;
}

void Server::sendTo(int clientId, const std::vector<uint8_t>& data, bool reliable){
    for(ServerTransport* transport : mTransports){
        transport->sendTo(clientId, data, reliable);
    }
}

void Server::broadcast(const std::vector<uint8_t>& data, bool reliable){
    for(ServerTransport* transport : mTransports){
        transport->broadcast(data, reliable);
    }
}

void Server::update(float realDeltaTime){
    if(mTransports.empty()) return;
    
    if(mInputs.size() != mWorld.playerManager.players.size()) mInputs.resize(mWorld.playerManager.players.size());

    for(ServerTransport* transport : mTransports){
        for(int clientId : transport->takeConnected()){
            if(!mWelcome.empty()) transport->sendTo(clientId, mWelcome, true);
        }

        transport->takeDisconnected();
    }

    for(ServerTransport* transport : mTransports){
        for(const NetMessage& message : transport->receive()){
            if(message.data.empty()) continue;

            ByteReader r(message.data.data(), message.data.size());
            MsgType type = (MsgType)r.u8();
            if(!r.ok()) continue;

            if(type == MsgType::JoinPlayer){
                if(readJoinPlayer(r)) mJoinRequests.push_back(message.clientId);
            }else if(type == MsgType::Input){
                std::vector<uint8_t> indices;
                std::vector<PlayerInput> inputs;

                if(!readInputMessage(r, indices, inputs)) continue;

                for(size_t i = 0; i < indices.size(); i++){
                    uint8_t index = indices[i];
                    if(index >= mInputs.size()) continue;

                    if(!owns(message.clientId, index)){
                        printf("[net] input rejected : client %d does not own player %u\n", message.clientId, index);
                        continue;
                    }

                    mInputs[index] = inputs[i];
                }
            }
        }
    }

    mAccumulator += realDeltaTime;

    int steps = 0;
    while(mAccumulator >= FIXED_DT){
        if(steps >= MAX_STEPS_PER_FRAME){
            mAccumulator = 0.f;
            break;
        }

        mWorld.step(FIXED_DT, mInputs);

        mAccumulator -= FIXED_DT;
        steps++;

        mSnapshotCounter++;
        if(mSnapshotCounter >= mSnapshotInterval){
            mSnapshotCounter = 0;

            ByteWriter w;
            writeSnapshotMessage(w, captureSnapshot(mWorld));

            for(ServerTransport* transport : mTransports){
                transport->broadcast(w.data(), false);
            }

            mWorld.events.clear();
        }
    }
}


