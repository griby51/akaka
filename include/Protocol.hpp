#pragma once

#include <cstdint>
#include <vector>

#include "ByteBuffer.hpp"
#include "PlayerInput.hpp"
#include "Snapshot.hpp"

enum class MsgType : uint8_t {
    Join = 1,
    Welcome = 2,
    Input = 3,
    Snapshot = 4,
};

constexpr uint16_t MAX_NET_PLAYERS = 8;
constexpr uint16_t MAX_NET_PROJECTILES = 1024;
constexpr uint16_t MAX_NET_EVENTS = 256;
constexpr uint16_t PROTOCOL_VERSION = 1;

void writeInput(ByteWriter& w, const PlayerInput& in);
bool readInput(ByteReader& r, PlayerInput& out);

void writeSnapshotMessage(ByteWriter& w, const Snapshot& snap);
void writeSnapshot(ByteWriter& w, const Snapshot& snap);
bool readSnapshot(ByteReader& r, Snapshot& out);

void writeInputMessage(ByteWriter& w, const std::vector<uint8_t>& indices, const std::vector<PlayerInput>& inputs);
bool readInputMessage(ByteReader& r, std::vector<uint8_t>& outIndices, std::vector<PlayerInput>& outInputs);
