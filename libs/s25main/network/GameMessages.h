// Copyright (C) 2005 - 2025 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "GameMessage.h"
#include "GameMessageInterface.h"
#include "GameMessage_Chat.h"
#include "GameProtocol.h"
#include "GlobalGameSettings.h"
#include "helpers/serializeEnums.h"
#include "random/Random.h"
#include "gameTypes/AIInfo.h"
#include "gameTypes/MapType.h"
#include "gameTypes/Nation.h"
#include "gameTypes/PlayerState.h"
#include "gameTypes/ServerType.h"
#include "gameTypes/TeamTypes.h"
#include "s25util/Log.h"
#include "s25util/Serializer.h"
#include <chrono>
#include <utility>

struct JoinPlayerInfo;
class MessageInterface;

/*
 * The class comment is from client side. For server side reverse the meaning
 *
 * Default ctor (no params) is mainly for receiving.
 * Ctor with params is for sending. Messages from the client don't need the player (ignored on server)
 */

/// eingehende Ping-Nachricht
class GameMessage_Ping : public GameMessageWithPlayer
{
public:
    GameMessage_Ping() : GameMessageWithPlayer(NMS_PING) {}
    GameMessage_Ping(uint8_t player) : GameMessageWithPlayer(NMS_PING, player) {}

    bool Run(GameMessageInterface* callback) const override
    {
        // LOG.writeToFile("<<< NMS_PING\n");
        return callback->OnGameMessage(*this);
    }
};

/// ausgehende Pong-Nachricht
class GameMessage_Pong : public GameMessage
{
public:
    GameMessage_Pong() : GameMessage(NMS_PONG) {}
    bool Run(GameMessageInterface* callback) const override
    {
        // LOG.writeToFile("<<< NMS_PONG\n");
        return callback->OnGameMessage(*this);
    }
};

/// ausgehende Server-Typ-Nachricht
class GameMessage_Server_Type : public GameMessage
{
public:
    ServerType type;
    std::string revision;

    GameMessage_Server_Type() : GameMessage(NMS_SERVER_TYPE) {}
    GameMessage_Server_Type(const ServerType type, const std::string& revision)
        : GameMessage(NMS_SERVER_TYPE), type(type), revision(revision)
    {
        LOG.writeToFile(">>> NMS_SERVER_Type(%d, %s)\n") % static_cast<int>(type) % revision;
    }

    void Serialize(Serializer& ser) const override
    {
        GameMessage::Serialize(ser);
        helpers::pushEnum<uint16_t>(ser, type);
        ser.PushLongString(revision);
    }

    void Deserialize(Serializer& ser) override
    {
        GameMessage::Deserialize(ser);
        type = helpers::popEnum<ServerType>(ser);
        revision = ser.PopLongString();
    }

    bool Run(GameMessageInterface* callback) const override
    {
        LOG.writeToFile("<<< NMS_SERVER_Type(%d, %s)\n") % static_cast<int>(type) % revision;
        return callback->OnGameMessage(*this);
    }
};

class GameMessage_Server_TypeOK : public GameMessage
{
public:
    enum class StatusCode : uint32_t
    {
        Ok,
        InvalidServerType,
        WrongVersion
    };
    friend constexpr auto maxEnumValue(StatusCode) { return StatusCode::WrongVersion; }

    /// Vom Server akzeptiert?
    StatusCode err_code;
    std::string version;

    GameMessage_Server_TypeOK() : GameMessage(NMS_SERVER_TYPEOK) {} //-V730
    GameMessage_Server_TypeOK(const StatusCode err_code, std::string version)
        : GameMessage(NMS_SERVER_TYPEOK), err_code(err_code), version(std::move(version))
    {}

    void Serialize(Serializer& ser) const override;
    void Deserialize(Serializer& ser) override;

    bool Run(GameMessageInterface* callback) const override { return callback->OnGameMessage(*this); }
};

/// ein/ausgehende Server-Password-Nachricht
class GameMessage_Server_Password : public GameMessage
{
public:
    std::string password;

    GameMessage_Server_Password() : GameMessage(NMS_SERVER_PASSWORD) {}
    GameMessage_Server_Password(std::string password) : GameMessage(NMS_SERVER_PASSWORD), password(std::move(password))
    {}

    void Serialize(Serializer& ser) const override
    {
        GameMessage::Serialize(ser);
        ser.PushString(password);
    }

    void Deserialize(Serializer& ser) override
    {
        GameMessage::Deserialize(ser);
        password = ser.PopString();
    }

    bool Run(GameMessageInterface* callback) const override
    {
        LOG.writeToFile("<<< NMS_SERVER_PASSWORD(%s)\n") % "********";
        return callback->OnGameMessage(*this);
    }
};

/// ausgehende Server-Name-Nachricht
class GameMessage_Server_Name : public GameMessage
{
public:
    std::string name;

    GameMessage_Server_Name() : GameMessage(NMS_SERVER_NAME) {}
    GameMessage_Server_Name(std::string name) : GameMessage(NMS_SERVER_NAME), name(std::move(name)) {}

    void Serialize(Serializer& ser) const override
    {
        LOG.writeToFile(">>> NMS_SERVER_NAME(%s)\n") % name;
        GameMessage::Serialize(ser);
        ser.PushString(name);
    }

    void Deserialize(Serializer& ser) override
    {
        GameMessage::Deserialize(ser);
        name = ser.PopString();
    }

    bool Run(GameMessageInterface* callback) const override
    {
        LOG.writeToFile("<<< NMS_SERVER_NAME(%s)\n") % name;
        return callback->OnGameMessage(*this);
    }
};

/// eingehende Server-Start-Nachricht
class GameMessage_Server_Start : public GameMessage
{
public:
    uint32_t random_init, firstNwf, cmdDelay;

    GameMessage_Server_Start() : GameMessage(NMS_SERVER_START) {} //-V730
    GameMessage_Server_Start(unsigned random_init, unsigned firstNwf, unsigned cmdDelay)
        : GameMessage(NMS_SERVER_START), random_init(random_init), firstNwf(firstNwf), cmdDelay(cmdDelay)
    {}

    void Serialize(Serializer& ser) const override
    {
        LOG.writeToFile(">>> NMS_SERVER_START(%d, %d, %d)\n") % random_init % firstNwf % cmdDelay;
        GameMessage::Serialize(ser);
        ser.PushUnsignedInt(random_init);
        ser.PushUnsignedInt(firstNwf);
        ser.PushUnsignedInt(cmdDelay);
    }

    void Deserialize(Serializer& ser) override
    {
        GameMessage::Deserialize(ser);
        random_init = ser.PopUnsignedInt();
        firstNwf = ser.PopUnsignedInt();
        cmdDelay = ser.PopUnsignedInt();
    }

    bool Run(GameMessageInterface* callback) const override
    {
        LOG.writeToFile("<<< NMS_SERVER_START(%d, %d, %d)\n") % random_init % firstNwf % cmdDelay;
        return callback->OnGameMessage(*this);
    }
};

/// C->S: Start countdown request
/// S->C: Current countdown
class GameMessage_Countdown : public GameMessage
{
public:
    uint32_t countdown;

    GameMessage_Countdown() : GameMessage(NMS_COUNTDOWN) {} //-V730
    GameMessage_Countdown(uint32_t countdown) : GameMessage(NMS_COUNTDOWN), countdown(countdown) {}

    void Serialize(Serializer& ser) const override
    {
        LOG.writeToFile(">>> NMS_SERVERCOUNTDOWN(%d)\n") % countdown;
        GameMessage::Serialize(ser);
        ser.PushUnsignedInt(countdown);
    }

    void Deserialize(Serializer& ser) override
    {
        GameMessage::Deserialize(ser);
        countdown = ser.PopUnsignedInt();
    }

    bool Run(GameMessageInterface* callback) const override
    {
        LOG.writeToFile("<<< NMS_SERVERCOUNTDOWN(%d)\n") % countdown;
        return callback->OnGameMessage(*this);
    }
};

/// Cancel countdown
class GameMessage_CancelCountdown : public GameMessage
{
public:
    bool error;
    GameMessage_CancelCountdown(bool error = false) : GameMessage(NMS_CANCEL_COUNTDOWN), error(error) {}

    void Serialize(Serializer& ser) const override
    {
        GameMessage::Serialize(ser);
        ser.PushBool(error);
    }

    void Deserialize(Serializer& ser) override
    {
        GameMessage::Deserialize(ser);
        error = ser.PopBool();
    }
    bool Run(GameMessageInterface* callback) const override { return callback->OnGameMessage(*this); }
};

/// eingehende Server-Async-Nachricht
class GameMessage_Server_Async : public GameMessage
{
public:
    std::vector<unsigned> checksums;

    GameMessage_Server_Async() : GameMessage(NMS_SERVER_ASYNC) {}
    GameMessage_Server_Async(const std::vector<unsigned>& checksums)
        : GameMessage(NMS_SERVER_ASYNC), checksums(checksums)
    {
        LOG.writeToFile(">>> NMS_SERVER_ASYNC(%d)\n") % checksums.size();
    }

    void Serialize(Serializer& ser) const override;

    void Deserialize(Serializer& ser) override;

    bool Run(GameMessageInterface* callback) const override
    {
        LOG.writeToFile("<<< NMS_SERVER_ASYNC(%d)\n") % checksums.size();
        return callback->OnGameMessage(*this);
    }
};

/// eingehende Player-ID-Nachricht
class GameMessage_Player_Id : public GameMessageWithPlayer
{
public:
    GameMessage_Player_Id() : GameMessageWithPlayer(NMS_PLAYER_ID) {} //-V730
    GameMessage_Player_Id(const uint8_t playerId) : GameMessageWithPlayer(NMS_PLAYER_ID, playerId)
    {
        LOG.writeToFile(">>> NMS_PLAYER_ID(%d)\n") % unsigned(playerId);
    }

    bool Run(GameMessageInterface* callback) const override
    {
        LOG.writeToFile("<<< NMS_PLAYER_ID(%d)\n") % unsigned(player);
        return callback->OnGameMessage(*this);
    }
};

/// ausgehende Player-Name-Nachricht
class GameMessage_Player_Name : public GameMessageWithPlayer
{
public:
    /// Name des neuen Spielers
    std::string playername;

    GameMessage_Player_Name() : GameMessageWithPlayer(NMS_PLAYER_NAME) {}
    GameMessage_Player_Name(uint8_t player, const std::string& playername)
        : GameMessageWithPlayer(NMS_PLAYER_NAME, player), playername(playername)
    {
        LOG.writeToFile(">>> NMS_PLAYER_NAME(%s)\n") % playername;
    }

    void Serialize(Serializer& ser) const override
    {
        GameMessageWithPlayer::Serialize(ser);
        ser.PushString(playername);
    }

    void Deserialize(Serializer& ser) override
    {
        GameMessageWithPlayer::Deserialize(ser);
        playername = ser.PopString();
    }

    bool Run(GameMessageInterface* callback) const override
    {
        LOG.writeToFile("<<< NMS_PLAYER_NAME(%s)\n") % playername;
        return callback->OnGameMessage(*this);
    }
};

class GameMessage_Player_Portrait : public GameMessageWithPlayer
{
public:
    unsigned int playerPortraitIndex;

    GameMessage_Player_Portrait() : GameMessageWithPlayer(NMS_PLAYER_PORTRAIT) {}
    GameMessage_Player_Portrait(uint8_t player, unsigned int portraitIndex)
        : GameMessageWithPlayer(NMS_PLAYER_PORTRAIT, player), playerPortraitIndex(portraitIndex)
    {}

    void Serialize(Serializer& ser) const override
    {
        GameMessageWithPlayer::Serialize(ser);
        ser.PushUnsignedInt(playerPortraitIndex);
    }

    void Deserialize(Serializer& ser) override
    {
        GameMessageWithPlayer::Deserialize(ser);
        playerPortraitIndex = ser.PopUnsignedInt();
    }

    bool Run(GameMessageInterface* callback) const override
    {
        LOG.writeToFile("<<< NMS_PLAYER_PORTRAIT(%u)\n") % playerPortraitIndex;
        return callback->OnGameMessage(*this);
    }
};

/// eingehende Player-List-Nachricht
class GameMessage_Player_List : public GameMessage
{
public:
    std::vector<JoinPlayerInfo> playerInfos;

    GameMessage_Player_List();
    GameMessage_Player_List(const std::vector<JoinPlayerInfo>& playerInfos);
    ~GameMessage_Player_List();

    void Serialize(Serializer& ser) const override;
    void Deserialize(Serializer& ser) override;
    bool Run(GameMessageInterface* callback) const override;
};

/// gehende -Nachricht
class GameMessage_Player_State : public GameMessageWithPlayer
{
public:
    PlayerState ps;
    AI::Info aiInfo;

    GameMessage_Player_State() : GameMessageWithPlayer(NMS_PLAYER_STATE) {} //-V730
    GameMessage_Player_State(uint8_t player, PlayerState ps, AI::Info aiInfo)
        : GameMessageWithPlayer(NMS_PLAYER_STATE, player), ps(ps), aiInfo(aiInfo)
    {
        LOG.writeToFile(">>> NMS_PLAYER_SETSTATE(%d)\n") % unsigned(player);
    }

    void Serialize(Serializer& ser) const override
    {
        GameMessageWithPlayer::Serialize(ser);
        helpers::pushEnum<uint8_t>(ser, ps);
        helpers::pushEnum<uint8_t>(ser, aiInfo.level);
        helpers::pushEnum<uint8_t>(ser, aiInfo.type);
    }

    void Deserialize(Serializer& ser) override
    {
        GameMessageWithPlayer::Deserialize(ser);
        ps = helpers::popEnum<PlayerState>(ser);
        aiInfo.level = helpers::popEnum<AI::Level>(ser);
        aiInfo.type = helpers::popEnum<AI::Type>(ser);
    }

    bool Run(GameMessageInterface* callback) const override
    {
        LOG.writeToFile("<<< NMS_PLAYER_SETSTATE(%d)\n") % unsigned(player);
        return callback->OnGameMessage(*this);
    }
};

/// gehende -Nachricht
class GameMessage_Player_Nation : public GameMessageWithPlayer
{
public:
    /// Das zu setzende Volk
    Nation nation;

    GameMessage_Player_Nation() : GameMessageWithPlayer(NMS_PLAYER_NATION) {} //-V730
    GameMessage_Player_Nation(uint8_t player, const Nation nation)
        : GameMessageWithPlayer(NMS_PLAYER_NATION, player), nation(nation)
    {
        LOG.writeToFile(">>> NMS_PLAYER_SET_NATION\n");
    }

    void Serialize(Serializer& ser) const override
    {
        GameMessageWithPlayer::Serialize(ser);
        helpers::pushEnum<uint8_t>(ser, nation);
    }

    void Deserialize(Serializer& ser) override
    {
        GameMessageWithPlayer::Deserialize(ser);
        nation = helpers::popEnum<Nation>(ser);
    }

    bool Run(GameMessageInterface* callback) const override
    {
        LOG.writeToFile("<<< NMS_PLAYER_SET_NATION\n");
        return callback->OnGameMessage(*this);
    }
};

class GameMessage_Player_Team : public GameMessageWithPlayer
{
public:
    /// Das zu setzende Team
    Team team;

    GameMessage_Player_Team() : GameMessageWithPlayer(NMS_PLAYER_TEAM) {} //-V730
    GameMessage_Player_Team(uint8_t player, const Team team)
        : GameMessageWithPlayer(NMS_PLAYER_TEAM, player), team(team)
    {
        LOG.writeToFile(">>> NMS_PLAYER_SET_TEAM\n");
    }

    void Serialize(Serializer& ser) const override
    {
        GameMessageWithPlayer::Serialize(ser);
        helpers::pushEnum<uint8_t>(ser, team);
    }

    void Deserialize(Serializer& ser) override
    {
        GameMessageWithPlayer::Deserialize(ser);
        team = helpers::popEnum<Team>(ser);
    }

    bool Run(GameMessageInterface* callback) const override
    {
        LOG.writeToFile("<<< NMS_PLAYER_SET_TEAM\n");
        return callback->OnGameMessage(*this);
    }
};

class GameMessage_Player_Color : public GameMessageWithPlayer
{
public:
    uint32_t color;

    GameMessage_Player_Color() : GameMessageWithPlayer(NMS_PLAYER_COLOR) {} //-V730
    GameMessage_Player_Color(uint8_t player, const uint32_t color)
        : GameMessageWithPlayer(NMS_PLAYER_COLOR, player), color(color)
    {
        LOG.writeToFile(">>> NMS_PLAYER_SET_COLOR\n");
    }

    void Serialize(Serializer& ser) const override
    {
        GameMessageWithPlayer::Serialize(ser);
        ser.PushUnsignedInt(color);
    }

    void Deserialize(Serializer& ser) override
    {
        GameMessageWithPlayer::Deserialize(ser);
        color = ser.PopUnsignedInt();
    }

    bool Run(GameMessageInterface* callback) const override
    {
        LOG.writeToFile("<<< NMS_PLAYER_SET_COLOR\n");
        return callback->OnGameMessage(*this);
    }
};

class GameMessage_Player_Kicked : public GameMessageWithPlayer
{
public:
    KickReason cause;
    /// Meta param to determine the origin of the kick
    uint32_t param;

    GameMessage_Player_Kicked() : GameMessageWithPlayer(NMS_PLAYER_KICKED) {} //-V730
    GameMessage_Player_Kicked(uint8_t player, KickReason cause, uint32_t param)
        : GameMessageWithPlayer(NMS_PLAYER_KICKED, player), cause(cause), param(param)
    {}

    void Serialize(Serializer& ser) const override
    {
        GameMessageWithPlayer::Serialize(ser);
        helpers::pushEnum<uint8_t>(ser, cause);
        ser.PushUnsignedInt(param);
    }

    void Deserialize(Serializer& ser) override
    {
        GameMessageWithPlayer::Deserialize(ser);
        cause = helpers::popEnum<KickReason>(ser);
        param = ser.PopUnsignedInt();
    }

    bool Run(GameMessageInterface* callback) const override { return callback->OnGameMessage(*this); }
};

/// gehende Player-Ping-Nachricht
class GameMessage_Player_Ping : public GameMessageWithPlayer
{
public:
    uint16_t ping;

    GameMessage_Player_Ping() : GameMessageWithPlayer(NMS_PLAYER_PING) {} //-V730
    GameMessage_Player_Ping(uint8_t player, uint16_t ping) : GameMessageWithPlayer(NMS_PLAYER_PING, player), ping(ping)
    {}

    void Serialize(Serializer& ser) const override
    {
        GameMessageWithPlayer::Serialize(ser);
        ser.PushUnsignedShort(ping);
    }

    void Deserialize(Serializer& ser) override
    {
        GameMessageWithPlayer::Deserialize(ser);
        ping = ser.PopUnsignedShort();
    }

    bool Run(GameMessageInterface* callback) const override
    {
        // LOG.writeToFile("<<< NMS_PLAYER_PING\n");
        return callback->OnGameMessage(*this);
    }
};

/// gehende Player-New-Nachricht
class GameMessage_Player_New : public GameMessageWithPlayer
{
public:
    /// Name des Neuen Spielers
    std::string name;

    GameMessage_Player_New() : GameMessageWithPlayer(NMS_PLAYER_NEW) {}
    GameMessage_Player_New(uint8_t player, std::string name)
        : GameMessageWithPlayer(NMS_PLAYER_NEW, player), name(std::move(name))
    {
        LOG.writeToFile(">>> NMS_PLAYER_NEW\n");
    }

    void Serialize(Serializer& ser) const override
    {
        GameMessageWithPlayer::Serialize(ser);
        ser.PushString(name);
    }

    void Deserialize(Serializer& ser) override
    {
        GameMessageWithPlayer::Deserialize(ser);
        name = ser.PopString();
    }

    bool Run(GameMessageInterface* callback) const override
    {
        LOG.writeToFile("<<< NMS_PLAYER_NEW\n");
        return callback->OnGameMessage(*this);
    }
};

/// gehende Player-Ready-Nachricht
class GameMessage_Player_Ready : public GameMessageWithPlayer
{
public:
    /// Ist der Spieler bereit?
    bool ready;

    GameMessage_Player_Ready() : GameMessageWithPlayer(NMS_PLAYER_READY) {} //-V730
    GameMessage_Player_Ready(uint8_t player, bool ready) : GameMessageWithPlayer(NMS_PLAYER_READY, player), ready(ready)
    {
        LOG.writeToFile(">>> NMS_PLAYER_READY\n");
    }

    void Serialize(Serializer& ser) const override
    {
        GameMessageWithPlayer::Serialize(ser);
        ser.PushBool(ready);
    }

    void Deserialize(Serializer& ser) override
    {
        GameMessageWithPlayer::Deserialize(ser);
        ready = ser.PopBool();
    }

    bool Run(GameMessageInterface* callback) const override
    {
        LOG.writeToFile("<<< NMS_PLAYER_READY\n");
        return callback->OnGameMessage(*this);
    }
};

/// Swap the given players
class GameMessage_Player_Swap : public GameMessageWithPlayer
{
public:
    /// Die beiden Spieler-IDs, die miteinander vertauscht werden sollen
    uint8_t player2;
    GameMessage_Player_Swap() : GameMessageWithPlayer(NMS_PLAYER_SWAP) {} //-V730
    GameMessage_Player_Swap(uint8_t player, uint8_t player2)
        : GameMessageWithPlayer(NMS_PLAYER_SWAP, player), player2(player2)
    {
        LOG.writeToFile(">>> NMS_PLAYER_SWAP\n");
    }

    void Serialize(Serializer& ser) const override
    {
        GameMessageWithPlayer::Serialize(ser);
        ser.PushUnsignedChar(player2);
    }

    void Deserialize(Serializer& ser) override
    {
        GameMessageWithPlayer::Deserialize(ser);
        player2 = ser.PopUnsignedChar();
    }

    bool Run(GameMessageInterface* callback) const override
    {
        LOG.writeToFile("<<< NMS_PLAYER_SWAP\n");
        return callback->OnGameMessage(*this);
    }
};

// Outgoing confirmation of the swap
class GameMessage_Player_SwapConfirm : public GameMessageWithPlayer
{
public:
    /// Die beiden Spieler-IDs, die miteinander vertauscht werden sollen
    uint8_t player2;
    GameMessage_Player_SwapConfirm() : GameMessageWithPlayer(NMS_PLAYER_SWAP_CONFIRM) {} //-V730
    GameMessage_Player_SwapConfirm(uint8_t player, uint8_t player2)
        : GameMessageWithPlayer(NMS_PLAYER_SWAP_CONFIRM, player), player2(player2)
    {}

    void Serialize(Serializer& ser) const override
    {
        GameMessageWithPlayer::Serialize(ser);
        ser.PushUnsignedChar(player2);
    }

    void Deserialize(Serializer& ser) override
    {
        GameMessageWithPlayer::Deserialize(ser);
        player2 = ser.PopUnsignedChar();
    }

    bool Run(GameMessageInterface* callback) const override { return callback->OnGameMessage(*this); }
};

class GameMessage_Map_Info : public GameMessage
{
public:
    /// Name of the map file
    std::string filename;
    /// Kartentyp (alte Karte neue Karte, Savegame usw.)
    MapType mt;
    uint32_t mapLen, mapCompressedLen;
    uint32_t luaLen, luaCompressedLen;

    GameMessage_Map_Info() : GameMessage(NMS_MAP_INFO) {} //-V730
    GameMessage_Map_Info(std::string filename, const MapType mt, unsigned mapLen, unsigned mapCompressedLen,
                         const unsigned luaLen, unsigned luaCompressedLen)
        : GameMessage(NMS_MAP_INFO), filename(std::move(filename)), mt(mt), mapLen(mapLen),
          mapCompressedLen(mapCompressedLen), luaLen(luaLen), luaCompressedLen(luaCompressedLen)
    {
        LOG.writeToFile(">>> NMS_MAP_INFO\n");
    }

    void Serialize(Serializer& ser) const override
    {
        GameMessage::Serialize(ser);
        ser.PushString(filename);
        helpers::pushEnum<uint8_t>(ser, mt);
        ser.PushUnsignedInt(mapLen);
        ser.PushUnsignedInt(mapCompressedLen);
        ser.PushUnsignedInt(luaLen);
        ser.PushUnsignedInt(luaCompressedLen);
    }

    void Deserialize(Serializer& ser) override
    {
        GameMessage::Deserialize(ser);
        filename = ser.PopString();
        mt = MapType(ser.PopUnsignedChar());
        mapLen = ser.PopUnsignedInt();
        mapCompressedLen = ser.PopUnsignedInt();
        luaLen = ser.PopUnsignedInt();
        luaCompressedLen = ser.PopUnsignedInt();
    }

    bool Run(GameMessageInterface* callback) const override
    {
        LOG.writeToFile("<<< NMS_MAP_INFO\n");
        return callback->OnGameMessage(*this);
    }
};

class GameMessage_MapRequest : public GameMessage
{
public:
    bool requestInfo;

    GameMessage_MapRequest() : GameMessage(NMS_MAP_REQUEST) {} //-V730
    GameMessage_MapRequest(bool requestInfo) : GameMessage(NMS_MAP_REQUEST), requestInfo(requestInfo) {}

    void Serialize(Serializer& ser) const override
    {
        GameMessage::Serialize(ser);
        ser.PushBool(requestInfo);
    }

    void Deserialize(Serializer& ser) override
    {
        GameMessage::Deserialize(ser);
        requestInfo = ser.PopBool();
    }

    bool Run(GameMessageInterface* callback) const override { return callback->OnGameMessage(*this); }
};

class GameMessage_Map_Data : public GameMessage
{
public:
    bool isMapData;
    /// Offset into map buffer
    uint32_t offset;
    /// Kartendaten
    std::vector<char> data;
    /// True for map data, false for luaData

    GameMessage_Map_Data() : GameMessage(NMS_MAP_DATA) {} //-V730
    GameMessage_Map_Data(bool isMapData, const uint32_t offset, const char* const data, unsigned length)
        : GameMessage(NMS_MAP_DATA), isMapData(isMapData), offset(offset), data(data, data + length)
    {
        LOG.writeToFile(">>> NMS_MAP_DATA\n");
    }

    void Serialize(Serializer& ser) const override
    {
        GameMessage::Serialize(ser);
        ser.PushBool(isMapData);
        ser.PushUnsignedInt(offset);
        ser.PushUnsignedInt(data.size());
        ser.PushRawData(&data.front(), data.size());
    }

    void Deserialize(Serializer& ser) override
    {
        GameMessage::Deserialize(ser);
        isMapData = ser.PopBool();
        offset = ser.PopUnsignedInt();
        data.resize(ser.PopUnsignedInt());
        ser.PopRawData(&data.front(), data.size());
    }

    bool Run(GameMessageInterface* callback) const override
    {
        LOG.writeToFile("<<< NMS_MAP_DATA\n");
        return callback->OnGameMessage(*this);
    }
};

class GameMessage_Map_Checksum : public GameMessage
{
public:
    /// Checksumme, die vom Client berechnet wurde
    uint32_t mapChecksum, luaChecksum;

    GameMessage_Map_Checksum() : GameMessage(NMS_MAP_CHECKSUM) {} //-V730
    GameMessage_Map_Checksum(uint32_t mapChecksum, uint32_t luaChecksum)
        : GameMessage(NMS_MAP_CHECKSUM), mapChecksum(mapChecksum), luaChecksum(luaChecksum)
    {
        LOG.writeToFile(">>> NMS_MAP_CHECKSUM\n");
    }

    void Serialize(Serializer& ser) const override
    {
        GameMessage::Serialize(ser);
        ser.PushUnsignedInt(mapChecksum);
        ser.PushUnsignedInt(luaChecksum);
    }

    void Deserialize(Serializer& ser) override
    {
        GameMessage::Deserialize(ser);
        mapChecksum = ser.PopUnsignedInt();
        luaChecksum = ser.PopUnsignedInt();
    }

    bool Run(GameMessageInterface* callback) const override
    {
        LOG.writeToFile("<<< NMS_MAP_CHECKSUM\n");
        return callback->OnGameMessage(*this);
    }
};

/// MapChecksum vom Server erfolgreich verifiziert?
class GameMessage_Map_ChecksumOK : public GameMessage
{
public:
    /// Vom Server akzeptiert?
    bool correct, retryAllowed;

    GameMessage_Map_ChecksumOK() : GameMessage(NMS_MAP_CHECKSUMOK) {} //-V730
    GameMessage_Map_ChecksumOK(bool correct, bool retryAllowed)
        : GameMessage(NMS_MAP_CHECKSUMOK), correct(correct), retryAllowed(retryAllowed)
    {}

    void Serialize(Serializer& ser) const override
    {
        GameMessage::Serialize(ser);
        ser.PushBool(correct);
        ser.PushBool(retryAllowed);
    }

    void Deserialize(Serializer& ser) override
    {
        GameMessage::Deserialize(ser);
        correct = ser.PopBool();
        retryAllowed = ser.PopBool();
    }

    bool Run(GameMessageInterface* callback) const override { return callback->OnGameMessage(*this); }
};

class GameMessage_GGSChange : public GameMessage
{
public:
    /// GGS-Daten
    GlobalGameSettings ggs;

    GameMessage_GGSChange() : GameMessage(NMS_GGS_CHANGE) {}
    GameMessage_GGSChange(GlobalGameSettings ggs) : GameMessage(NMS_GGS_CHANGE), ggs(std::move(ggs))
    {
        LOG.writeToFile(">>> NMS_GGS_CHANGE\n");
    }

    void Serialize(Serializer& ser) const override
    {
        GameMessage::Serialize(ser);
        ggs.Serialize(ser);
    }

    void Deserialize(Serializer& ser) override
    {
        GameMessage::Deserialize(ser);
        ggs.Deserialize(ser);
    }

    bool Run(GameMessageInterface* callback) const override
    {
        LOG.writeToFile("<<< NMS_GGS_CHANGE\n");
        return callback->OnGameMessage(*this);
    }
};

class GameMessage_RemoveLua : public GameMessage
{
public:
    GameMessage_RemoveLua() : GameMessage(NMS_REMOVE_LUA) {}

    bool Run(GameMessageInterface* callback) const override { return callback->OnGameMessage(*this); }
};

class GameMessage_Speed : public GameMessage
{
public:
    std::chrono::milliseconds gf_length; // new speed

    GameMessage_Speed() : GameMessage(NMS_SERVER_SPEED) {} //-V730
    GameMessage_Speed(const std::chrono::milliseconds gf_length) : GameMessage(NMS_SERVER_SPEED), gf_length(gf_length)
    {
        LOG.writeToFile(">>> NMS_SERVER_SPEED(%d)\n") % gf_length.count();
    }

    void Serialize(Serializer& ser) const override
    {
        GameMessage::Serialize(ser);
        ser.PushUnsignedInt(static_cast<uint32_t>(gf_length.count()));
    }

    void Deserialize(Serializer& ser) override
    {
        GameMessage::Deserialize(ser);
        gf_length = decltype(gf_length)(ser.PopUnsignedInt());
    }

    bool Run(GameMessageInterface* callback) const override
    {
        LOG.writeToFile("<<< NMS_SERVER_SPEED(%d)\n") % gf_length.count();
        return callback->OnGameMessage(*this);
    }
};

class GameMessage_Server_NWFDone : public GameMessage
{
public:
    uint32_t gf, gf_length, nextNWF;

    GameMessage_Server_NWFDone() : GameMessage(NMS_SERVER_NWF_DONE) {} //-V730
    GameMessage_Server_NWFDone(unsigned gf, unsigned gf_length, unsigned nextNWF)
        : GameMessage(NMS_SERVER_NWF_DONE), gf(gf), gf_length(gf_length), nextNWF(nextNWF)
    {
        LOG.writeToFile(">>> NMS_NWF_DONE(%d, %d, %d)\n") % gf % gf_length % nextNWF;
    }

    void Serialize(Serializer& ser) const override
    {
        GameMessage::Serialize(ser);
        ser.PushUnsignedInt(gf);
        ser.PushUnsignedInt(gf_length);
        ser.PushUnsignedInt(nextNWF);
    }

    void Deserialize(Serializer& ser) override
    {
        GameMessage::Deserialize(ser);
        gf = ser.PopUnsignedInt();
        gf_length = ser.PopUnsignedInt();
        nextNWF = ser.PopUnsignedInt();
    }

    bool Run(GameMessageInterface* callback) const override
    {
        LOG.writeToFile("<<< NMS_NWF_DONE(%d, %d, %d)\n") % gf % gf_length % nextNWF;
        return callback->OnGameMessage(*this);
    }
};

class GameMessage_Pause : public GameMessage
{
public:
    /// Pausiert?
    bool paused;

    GameMessage_Pause() : GameMessage(NMS_PAUSE) {} //-V730
    GameMessage_Pause(const bool paused) : GameMessage(NMS_PAUSE), paused(paused)
    {
        LOG.writeToFile(">>> NMS_PAUSE(%d)\n") % (paused ? 1 : 0);
    }

    void Serialize(Serializer& ser) const override
    {
        GameMessage::Serialize(ser);
        ser.PushBool(paused);
    }

    void Deserialize(Serializer& ser) override
    {
        GameMessage::Deserialize(ser);
        paused = ser.PopBool();
    }

    bool Run(GameMessageInterface* callback) const override
    {
        LOG.writeToFile("<<< NMS_PAUSE(%d)\n") % (paused ? 1 : 0);
        return callback->OnGameMessage(*this);
    }
};

class GameMessage_SkipToGF : public GameMessage
{
public:
    uint32_t targetGF;

    GameMessage_SkipToGF() : GameMessage(NMS_SKIP_TO_GF) {} //-V730
    GameMessage_SkipToGF(uint32_t targetGF) : GameMessage(NMS_SKIP_TO_GF), targetGF(targetGF) {}

    void Serialize(Serializer& ser) const override
    {
        GameMessage::Serialize(ser);
        ser.PushUnsignedInt(targetGF);
    }

    void Deserialize(Serializer& ser) override
    {
        GameMessage::Deserialize(ser);
        targetGF = ser.PopUnsignedInt();
    }

    bool Run(GameMessageInterface* callback) const override { return callback->OnGameMessage(*this); }
};

/// ausgehende GetAsyncLog-Nachricht
class GameMessage_GetAsyncLog : public GameMessage
{
public:
    GameMessage_GetAsyncLog() : GameMessage(NMS_GET_ASYNC_LOG) {}
    bool Run(GameMessageInterface* callback) const override { return callback->OnGameMessage(*this); }
};

/// eingehende SendAsyncLog-Nachricht
class GameMessage_AsyncLog : public GameMessage
{
public:
    std::string addData;
    std::vector<RandomEntry> entries;
    bool last;

    GameMessage_AsyncLog() : GameMessage(NMS_ASYNC_LOG) {} //-V730

    GameMessage_AsyncLog(std::string addData) : GameMessage(NMS_ASYNC_LOG), addData(std::move(addData)), last(false)
    {
        LOG.writeToFile(">>> NMS_SEND_ASYNC_LOG\n");
    }

    GameMessage_AsyncLog(std::vector<RandomEntry> async_log, bool last)
        : GameMessage(NMS_ASYNC_LOG), entries(std::move(async_log)), last(last)
    {
        LOG.writeToFile(">>> NMS_SEND_ASYNC_LOG\n");
    }

    void Serialize(Serializer& ser) const override
    {
        GameMessage::Serialize(ser);
        ser.PushString(addData);

        ser.PushUnsignedInt(entries.size());
        for(const RandomEntry& entry : entries)
            entry.Serialize(ser);

        ser.PushBool(last);
    }

    void Deserialize(Serializer& ser) override
    {
        GameMessage::Deserialize(ser);
        addData += ser.PopString();

        unsigned cnt = ser.PopUnsignedInt();
        entries.clear();
        entries.resize(cnt);
        for(RandomEntry& entry : entries)
            entry.Deserialize(ser);

        last = ser.PopBool();
    }

    bool Run(GameMessageInterface* callback) const override
    {
        LOG.writeToFile("<<< NMS_SEND_ASYNC_LOG: %u [%s]\n") % entries.size() % (last ? "last" : "non-last");
        return callback->OnGameMessage(*this);
    }
};
