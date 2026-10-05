#include "Server.hpp"
#include "ByteBuffer.hpp"
#include "Snapshot.hpp"
#include "Transport.hpp"
#include "World.hpp"
#include "Protocol.hpp"
#include <cstdint>

const World& Server::world() const{ return mWorld; }
World& Server::world(){ return mWorld; }

void Server::setTransport(ServerTransport* transport){mTransport = transport;}

void Server::update(float realDeltaTime){
    if(!mTransport) return;
    
    if(mInputs.size() != mWorld.playerManager.players.size()) mInputs.resize(mWorld.playerManager.players.size());

    for(const NetMessage& message : mTransport->receive()){
        if(message.data.empty()) continue;

        ByteReader r(message.data.data(), message.data.size());
        MsgType type = (MsgType)r.u8();
        if(!r.ok()) continue;

        if(type == MsgType::Input){
            std::vector<uint8_t> indices;
            std::vector<PlayerInput> inputs;

            if(!readInputMessage(r, indices, inputs)) continue;

            for(size_t i = 0; i < indices.size(); i++){
                uint8_t index = indices[i];
                if(index >= mInputs.size()) continue;

                mInputs[index] = inputs[i];
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
            mTransport->broadcast(w.data(), false);

            mWorld.events.clear();
        }
    }
}


