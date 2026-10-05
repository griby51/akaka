#pragma once

#include <cstdint>
#include <vector>

#include "ByteBuffer.hpp"
#include "Lobby.hpp"
#include "PlayerInfo.hpp"
#include "PlayerInput.hpp"
#include "Snapshot.hpp"

enum class MsgType : uint8_t {
    Join = 1,
    Welcome = 2,
    Input = 3,
    Snapshot = 4,
    JoinPlayer = 5,
    Ownership = 6,
    LobbyState = 7,
    SlotUpdate = 8,
    SlotAssigned = 9,
    StartMatch = 10,
};

constexpr uint16_t MAX_NET_PLAYERS = 8;
constexpr uint16_t MAX_NET_PROJECTILES = 1024;
constexpr uint16_t MAX_NET_EVENTS = 256;
constexpr uint16_t MAX_NET_ASSETS = 4096;
constexpr uint16_t PROTOCOL_VERSION = 1;

struct WelcomeData{
    std::vector<PlayerInfo> players;
    std::vector<std::string> assets;
};

void writeWelcomeMessage(ByteWriter& w, const WelcomeData& data);
bool readWelcome(ByteReader& r, WelcomeData& out);

struct LobbySlot;

void writeLobbyStateMessage(ByteWriter& w, const std::vector<LobbySlot>& slots);
void writeStartMatchMessage(ByteWriter& w, const std::vector<LobbySlot>& slots);
bool readLobbySlots(ByteReader& r, std::vector<LobbySlot>& out);

void writeSlotUpdateMessage(ByteWriter& w, uint8_t slotIndex, uint8_t skinIndex, uint8_t hatIndex, bool ready);
bool readSlotUpdate(ByteReader& r, uint8_t& slotIndex, uint8_t& skinIndex, uint8_t& hatIndex, bool& ready);

void writeSlotAssignedMessage(ByteWriter& w, uint8_t slotIndex);
bool readSlotAssigned(ByteReader& r, uint8_t& slotIndex);

void writeJoinPlayerMessage(ByteWriter& w);
bool readJoinPlayer(ByteReader& r);

void writeOwnershipMessage(ByteWriter& w, const std::vector<uint8_t>& indices);
bool readOwnership(ByteReader& r, std::vector<uint8_t>& out);

void writeInput(ByteWriter& w, const PlayerInput& in);
bool readInput(ByteReader& r, PlayerInput& out);

void writeSnapshotMessage(ByteWriter& w, const Snapshot& snap);
void writeSnapshot(ByteWriter& w, const Snapshot& snap);
bool readSnapshot(ByteReader& r, Snapshot& out);

void writeInputMessage(ByteWriter& w, const std::vector<uint8_t>& indices, const std::vector<PlayerInput>& inputs);
bool readInputMessage(ByteReader& r, std::vector<uint8_t>& outIndices, std::vector<PlayerInput>& outInputs);
