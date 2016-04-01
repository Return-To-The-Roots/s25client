// Copyright (c) 2005 - 2015 Settlers Freaks (sf-team at siedler25.org)
//
// This file is part of Return To The Roots.
//
// Return To The Roots is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
//
// Return To The Roots is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Return To The Roots. If not, see <http://www.gnu.org/licenses/>.
#ifndef GAMEMESSAGES_H_INCLUDED
#define GAMEMESSAGES_H_INCLUDED

#include "GameMessage.h"
#include "GameMessageInterface.h"
#include "GameProtocol.h"
#include "GamePlayerList.h"
#include "GlobalGameSettings.h"
#include "Random.h"
#include "../libutil/src/Log.h"
class MessageInterface;
/*
 * das Klassenkommentar ist alles Client-Sicht, für Server-Sicht ist alles andersrum
 *
 * Konstruktor ohne Parameter ist allgemein nur zum Empfangen (immer noop!)
 * run-Methode ist Auswertung der Daten
 *
 * Konstruktor(en) mit Parametern (wenns auch nur der "reserved"-Parameter ist)
 * ist zum Verschicken der Nachrichten gedacht!
 */

///////////////////////////////////////////////////////////////////////////////
/// eingehende Ping-Nachricht
class GameMessage_Ping : public GameMessage
{
public:
	GameMessage_Ping(): GameMessage(NMS_PING) {}
	GameMessage_Ping(const unsigned char player): GameMessage(NMS_PING, player) {}

	void Run(MessageInterface* callback) override
	{
		//LOG.write("<<< NMS_PING\n");
		GetInterface(callback)->OnGameMessage(*this);
	}
};

///////////////////////////////////////////////////////////////////////////////
/// ausgehende Pong-Nachricht
class GameMessage_Pong : public GameMessage
{
public:
	GameMessage_Pong(): GameMessage(NMS_PONG) { }
	GameMessage_Pong(const unsigned char player): GameMessage(NMS_PONG, player) {}
	void Run(MessageInterface* callback) override
	{
		//LOG.write("<<< NMS_PONG\n");
		GetInterface(callback)->OnGameMessage(*this);
	}
};

///////////////////////////////////////////////////////////////////////////////
/// ausgehende Server-Typ-Nachricht
class GameMessage_Server_Type: public GameMessage
{
public:
	ServerType type;
	std::string version;

	GameMessage_Server_Type(): GameMessage(NMS_SERVER_TYPE) { }
	GameMessage_Server_Type(const ServerType type, const std::string& version): GameMessage(NMS_SERVER_TYPE, 0xFF), type(type), version(version)
    {
        LOG.write(">>> NMS_SERVER_Type(%d, %s)\n", boost::underlying_cast<int>(type), version.c_str());    
    }

	void Serialize(Serializer& ser) const override
	{
		GameMessage::Serialize(ser);
		ser.PushUnsignedShort(boost::underlying_cast<unsigned short>(type));
		ser.PushString(version);
	}

    void Deserialize(Serializer& ser) override
	{
		GameMessage::Deserialize(ser);
        type = static_cast<ServerType>(ser.PopUnsignedShort());
        version = ser.PopString();
	}

	void Run(MessageInterface* callback) override
	{
		LOG.write("<<< NMS_SERVER_Type(%d, %s)\n", boost::underlying_cast<int>(type), version.c_str());
		GetInterface(callback)->OnGameMessage(*this);
	}
};


class GameMessage_Server_TypeOK: public GameMessage
{
public:
	/// Vom Server akzeptiert?
	unsigned err_code;

	GameMessage_Server_TypeOK(): GameMessage(NMS_SERVER_TYPEOK) { } //-V730
	GameMessage_Server_TypeOK(const unsigned err_code): GameMessage(NMS_SERVER_TYPEOK, 0xFF), err_code(err_code){}

	void Serialize(Serializer& ser) const override
    {
        GameMessage::Serialize(ser);
        ser.PushUnsignedInt(err_code);
    }

    void Deserialize(Serializer& ser) override
	{
		GameMessage::Deserialize(ser);
        err_code = ser.PopUnsignedInt();
	}

	void Run(MessageInterface* callback) override
	{
		GetInterface(callback)->OnGameMessage(*this);
	}
};


///////////////////////////////////////////////////////////////////////////////
/// ein/ausgehende Server-Password-Nachricht
class GameMessage_Server_Password : public GameMessage
{
public:
	std::string password;

	GameMessage_Server_Password(): GameMessage(NMS_SERVER_PASSWORD) { }
	GameMessage_Server_Password(const std::string& password): GameMessage(NMS_SERVER_PASSWORD, 0xFF), password(password){}

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

	void Run(MessageInterface* callback) override
	{
		LOG.write("<<< NMS_SERVER_PASSWORD(%s)\n", "********");
		GetInterface(callback)->OnGameMessage(*this);
	}
};

///////////////////////////////////////////////////////////////////////////////
/// ausgehende Server-Name-Nachricht
class GameMessage_Server_Name : public GameMessage
{
public:
	std::string name;

	GameMessage_Server_Name(): GameMessage(NMS_SERVER_NAME) { }
    GameMessage_Server_Name(const std::string& name): GameMessage(NMS_SERVER_NAME, 0xFF), name(name){}

	void Serialize(Serializer& ser) const override
    {
        LOG.write(">>> NMS_SERVER_NAME(%s)\n", name.c_str());
        GameMessage::Serialize(ser);
        ser.PushString(name);
    }

    void Deserialize(Serializer& ser) override
	{
		GameMessage::Deserialize(ser);
        name = ser.PopString();
	}

	void Run(MessageInterface* callback) override
	{
		LOG.write("<<< NMS_SERVER_NAME(%s)\n", name.c_str());
		GetInterface(callback)->OnGameMessage(*this);
	}
};

///////////////////////////////////////////////////////////////////////////////
/// eingehende Server-Start-Nachricht
class GameMessage_Server_Start : public GameMessage
{
public:
	unsigned int random_init;
	unsigned int nwf_length;

	GameMessage_Server_Start(): GameMessage(NMS_SERVER_START) { } //-V730
	GameMessage_Server_Start(const unsigned random_init, const unsigned nwf_length): GameMessage(NMS_SERVER_START, 0xFF), random_init(random_init), nwf_length(nwf_length){}

	void Serialize(Serializer& ser) const override
    {
        LOG.write(">>> NMS_SERVER_START(%d, %d)\n", random_init, nwf_length);
        GameMessage::Serialize(ser);
        ser.PushUnsignedInt(random_init);
        ser.PushUnsignedInt(nwf_length);
    }

    void Deserialize(Serializer& ser) override
	{
		GameMessage::Deserialize(ser);
        random_init = ser.PopUnsignedInt();
        nwf_length = ser.PopUnsignedInt();
	}

	void Run(MessageInterface* callback) override
	{
		LOG.write("<<< NMS_SERVER_START(%d, %d)\n", random_init, nwf_length);
		GetInterface(callback)->OnGameMessage(*this);
	}
};

///////////////////////////////////////////////////////////////////////////////
/// eingehende Server-Countdown-Nachricht
class GameMessage_Server_Countdown : public GameMessage
{
public:
	int countdown;

	GameMessage_Server_Countdown(): GameMessage(NMS_SERVER_COUNTDOWN) { } //-V730
	GameMessage_Server_Countdown(int countdown): GameMessage(NMS_SERVER_COUNTDOWN, 0xFF), countdown(countdown){}

	void Serialize(Serializer& ser) const override
    {
        LOG.write(">>> NMS_SERVER_COUNTDOWN(%d)\n", countdown);
        GameMessage::Serialize(ser);
        ser.PushUnsignedInt(countdown);
    }

    void Deserialize(Serializer& ser) override
	{
		GameMessage::Deserialize(ser);
        countdown = ser.PopUnsignedInt();
	}

	void Run(MessageInterface* callback) override
	{
		LOG.write("<<< NMS_SERVER_COUNTDOWN(%d)\n", countdown);
		GetInterface(callback)->OnGameMessage(*this);
	}
};

///////////////////////////////////////////////////////////////////////////////
/// eingehende Server-CancelCountdown-Nachricht
class GameMessage_Server_CancelCountdown : public GameMessage
{
public:
	GameMessage_Server_CancelCountdown(): GameMessage(NMS_SERVER_CANCELCOUNTDOWN) {}

	void Run(MessageInterface* callback) override
	{
		LOG.write("<<< NMS_SERVER_CANCELCOUNTDOWN\n");
		GetInterface(callback)->OnGameMessage(*this);
	}
};

///////////////////////////////////////////////////////////////////////////////
/// ein/ausgehende Server-Chat-Nachricht
class GameMessage_Server_Chat : public GameMessage
{
public:
    ChatDestination destination;
    std::string text;

    GameMessage_Server_Chat(): GameMessage(NMS_SERVER_CHAT) {} //-V730
    GameMessage_Server_Chat(const unsigned char player, const ChatDestination destination, const std::string& text):
        GameMessage(NMS_SERVER_CHAT, player), destination(destination), text(text)
    {
        LOG.write(">>> NMS_SERVER_CHAT(%d, %s)\n", destination, text.c_str());
    }

    void Serialize(Serializer& ser) const override
    {
        GameMessage::Serialize(ser);
        ser.PushUnsignedChar(static_cast<unsigned char>(destination));
        ser.PushString(text);
    }

    void Deserialize(Serializer& ser) override
    {
        GameMessage::Deserialize(ser);
        destination = ChatDestination(ser.PopUnsignedChar());
        text = ser.PopString();
    }

    void Run(MessageInterface* callback) override
    {
        LOG.write("<<< NMS_SERVER_CHAT(%d, %s)\n", destination, text.c_str());
        GetInterface(callback)->OnGameMessage(*this);
    }
};

class GameMessage_System_Chat : public GameMessage
{
public:
    std::string text;

    GameMessage_System_Chat(): GameMessage(NMS_SYSTEM_CHAT) { }
    GameMessage_System_Chat(const unsigned char player, const std::string& text): GameMessage(NMS_SYSTEM_CHAT, player), text(text){}

    void Serialize(Serializer& ser) const override
    {
        GameMessage::Serialize(ser);
        ser.PushString(text);
    }

    void Deserialize(Serializer& ser) override
    {
        GameMessage::Deserialize(ser);
        text = ser.PopString();
    }

    void Run(MessageInterface* callback) override
    {
        GetInterface(callback)->OnGameMessage(*this);
    }
};

///////////////////////////////////////////////////////////////////////////////
/// eingehende Server-Async-Nachricht
class GameMessage_Server_Async : public GameMessage
{
public:
    std::vector<int> checksums;

    GameMessage_Server_Async(): GameMessage(NMS_SERVER_ASYNC) { }
    GameMessage_Server_Async(const std::vector<int>& checksums): GameMessage(NMS_SERVER_ASYNC, 0xFF), checksums(checksums)
    {
        LOG.write(">>> NMS_SERVER_ASYNC(%d)\n", checksums.size());
    }

    void Serialize(Serializer& ser) const override
    {
         GameMessage::Serialize(ser);
         ser.PushUnsignedInt(unsigned(checksums.size()));
         for(unsigned int i = 0; i < checksums.size(); ++i)
             ser.PushSignedInt(checksums[i]);
    }

    void Deserialize(Serializer& ser) override
    {
        GameMessage::Deserialize(ser);
        unsigned size = ser.PopUnsignedInt();
        checksums.resize(size);
        for(unsigned int i = 0; i < size; ++i)
            checksums[i] = ser.PopSignedInt();
    }

    void Run(MessageInterface* callback) override
    {
        LOG.write("<<< NMS_SERVER_ASYNC(%d)\n", checksums.size());
        GetInterface(callback)->OnGameMessage(*this);
    }
};

///////////////////////////////////////////////////////////////////////////////
/// eingehende Player-ID-Nachricht
class GameMessage_Player_Id : public GameMessage
{
public:
    unsigned int playerid;

    GameMessage_Player_Id(): GameMessage(NMS_PLAYER_ID) { } //-V730
    GameMessage_Player_Id(const unsigned int playerid): GameMessage(NMS_PLAYER_ID, 0xFF), playerid(playerid)
    {
        LOG.write(">>> NMS_PLAYER_ID(%d)\n", playerid);
    }

    void Serialize(Serializer& ser) const override
    {
        GameMessage::Serialize(ser);
        ser.PushUnsignedInt(playerid);
    }

    void Deserialize(Serializer& ser) override
    {
        GameMessage::Deserialize(ser);
        playerid = ser.PopUnsignedInt();
    }

    void Run(MessageInterface* callback) override
    {
        LOG.write("<<< NMS_PLAYER_ID(%d)\n", playerid);
        GetInterface(callback)->OnGameMessage(*this);
    }
};

///////////////////////////////////////////////////////////////////////////////
/// ausgehende Player-Name-Nachricht
class GameMessage_Player_Name : public GameMessage
{
public:
    /// Name des neuen Spielers
    std::string playername;

    GameMessage_Player_Name(): GameMessage(NMS_PLAYER_NAME) { }
    GameMessage_Player_Name(const std::string& playername): GameMessage(NMS_PLAYER_NAME, 0xFF), playername(playername)
    {
        LOG.write(">>> NMS_PLAYER_NAME(%s)\n", playername.c_str());
    }

    void Serialize(Serializer& ser) const override
    {
        GameMessage::Serialize(ser);
        ser.PushString(playername);
    }

    void Deserialize(Serializer& ser) override
    {
        GameMessage::Deserialize(ser);
        playername = ser.PopString();
    }

    void Run(MessageInterface* callback) override
    {
        LOG.write("<<< NMS_PLAYER_NAME(%s)\n", playername.c_str());
        GetInterface(callback)->OnGameMessage(*this);
    }
};

///////////////////////////////////////////////////////////////////////////////
/// eingehende Player-List-Nachricht
class GameMessage_Player_List : public GameMessage
{
public:
	GamePlayerList gpl;

	GameMessage_Player_List(): GameMessage(NMS_PLAYER_LIST) { }
	GameMessage_Player_List(const GameServerPlayerList& gpl): GameMessage(NMS_PLAYER_LIST, 0xFF), gpl(gpl)
	{
		LOG.write(">>> NMS_PLAYER_LIST(%d)\n", gpl.getCount());
	}

	void Serialize(Serializer& ser) const override
    {
        GameMessage::Serialize(ser);
        gpl.serialize(ser);
    }

    void Deserialize(Serializer& ser) override
	{
		GameMessage::Deserialize(ser);
        gpl.deserialize(ser);
	}

	void Run(MessageInterface* callback) override
	{
		LOG.write("<<< NMS_PLAYER_LIST(%d)\n", gpl.getCount());
		for(unsigned int i = 0; i < gpl.getCount(); ++i)
		{
			const GamePlayerInfo* playerInfo = gpl.getElement(i);
			LOG.write("    %d: %s %d %d %d %d %d %d %s\n", i, playerInfo->name.c_str(), playerInfo->ps, playerInfo->rating, playerInfo->ping, playerInfo->nation, playerInfo->color, playerInfo->team, (playerInfo->ready ? "true" : "false") );
		}
		GetInterface(callback)->OnGameMessage(*this);
	}
};

///////////////////////////////////////////////////////////////////////////////
/// gehende -Nachricht
class GameMessage_Player_Set_State : public GameMessage
{
public:
    PlayerState ps;
    AI::Info aiInfo;

    GameMessage_Player_Set_State(): GameMessage(NMS_PLAYER_SETSTATE) { }
    GameMessage_Player_Set_State(const unsigned char player, PlayerState ps, AI::Info aiInfo): GameMessage(NMS_PLAYER_SETSTATE, player), ps(ps), aiInfo(aiInfo)
	{
		LOG.write(">>> NMS_PLAYER_SETSTATE(%d)\n", player);
	}

    void Serialize(Serializer& ser) const override
    {
        GameMessage::Serialize(ser);
        ser.PushUnsignedChar(static_cast<unsigned char>(ps));
        ser.PushUnsignedChar(static_cast<unsigned char>(aiInfo.level));
        ser.PushUnsignedChar(static_cast<unsigned char>(aiInfo.type));
    }

    void Deserialize(Serializer& ser) override
    {
        GameMessage::Deserialize(ser);
        ps = PlayerState(ser.PopUnsignedChar());
        aiInfo.level = AI::Level(ser.PopUnsignedChar());
        aiInfo.type = AI::Type(ser.PopUnsignedChar());
    }

	void Run(MessageInterface* callback) override
	{
		LOG.write("<<< NMS_PLAYER_SETSTATE(%d)\n", player);
		GetInterface(callback)->OnGameMessage(*this);
	}
};

///////////////////////////////////////////////////////////////////////////////
/// gehende -Nachricht
class GameMessage_Player_Set_Nation : public GameMessage
{
public:
	/// Das zu setzende Volk
	Nation nation;

	GameMessage_Player_Set_Nation(): GameMessage(NMS_PLAYER_SET_NATION) {} //-V730
	GameMessage_Player_Set_Nation(const unsigned char player, const Nation nation): GameMessage(NMS_PLAYER_SET_NATION, player), nation(nation)
	{
		LOG.write(">>> NMS_PLAYER_SET_NATION\n");
	}

	void Serialize(Serializer& ser) const override
    {
        GameMessage::Serialize(ser);
        ser.PushUnsignedChar(static_cast<unsigned char>(nation));
    }

    void Deserialize(Serializer& ser) override
	{
		GameMessage::Deserialize(ser);
        nation = Nation(ser.PopUnsignedChar());
	}

	void Run(MessageInterface* callback) override
	{
		LOG.write("<<< NMS_PLAYER_SET_NATION\n");
		GetInterface(callback)->OnGameMessage(*this);
	}
};

class GameMessage_Player_Set_Team : public GameMessage
{
public:
	/// Das zu setzende Team
	Team team;

	GameMessage_Player_Set_Team(): GameMessage(NMS_PLAYER_SET_TEAM) { } //-V730
	GameMessage_Player_Set_Team(const unsigned char player, const Team team): GameMessage(NMS_PLAYER_SET_TEAM, player), team(team)
	{
		LOG.write(">>> NMS_PLAYER_SET_TEAM\n");
	}

	void Serialize(Serializer& ser) const override
    {
        GameMessage::Serialize(ser);
        ser.PushUnsignedChar(static_cast<unsigned char>(team));
    }

    void Deserialize(Serializer& ser) override
	{
		GameMessage::Deserialize(ser);
        team = Team(ser.PopUnsignedChar());
    }

	void Run(MessageInterface* callback) override
	{
		LOG.write("<<< NMS_PLAYER_SET_TEAM\n");
		GetInterface(callback)->OnGameMessage(*this);
	}
};

class GameMessage_Player_Set_Color : public GameMessage
{
public:
	unsigned color;

	GameMessage_Player_Set_Color(): GameMessage(NMS_PLAYER_SET_COLOR) { } //-V730
	GameMessage_Player_Set_Color(const unsigned char player, const unsigned color): GameMessage(NMS_PLAYER_SET_COLOR, player), color(color)
	{
		LOG.write(">>> NMS_PLAYER_SET_COLOR\n");
	}

	void Serialize(Serializer& ser) const override
    {
        GameMessage::Serialize(ser);
        ser.PushUnsignedInt(color);
    }

    void Deserialize(Serializer& ser) override
	{
		GameMessage::Deserialize(ser);
        color = ser.PopUnsignedInt();
    }

	void Run(MessageInterface* callback) override
	{
		LOG.write("<<< NMS_PLAYER_SET_COLOR\n");
		GetInterface(callback)->OnGameMessage(*this);
	}
};

///////////////////////////////////////////////////////////////////////////////
/// gehende Player-Kicked-Nachricht
class GameMessage_Player_Kicked : public GameMessage
{
public:
	unsigned char cause;
	unsigned short param;

	GameMessage_Player_Kicked(): GameMessage(NMS_PLAYER_KICKED) { } //-V730
	GameMessage_Player_Kicked(const unsigned char player, const unsigned char cause, const unsigned short param)
		: GameMessage(NMS_PLAYER_KICKED, player), cause(cause), param(param){}

	void Serialize(Serializer& ser) const override
    {
        GameMessage::Serialize(ser);
        ser.PushUnsignedChar(cause);
        ser.PushUnsignedShort(param);
    }

    void Deserialize(Serializer& ser) override
	{
		GameMessage::Deserialize(ser);
        cause = ser.PopUnsignedChar();
        param = ser.PopUnsignedShort();
	}

	void Run(MessageInterface* callback) override
	{
		LOG.write("<<< NMS_PLAYER_KICKED\n");
		GetInterface(callback)->OnGameMessage(*this);
	}
};

///////////////////////////////////////////////////////////////////////////////
/// gehende Player-Ping-Nachricht
class GameMessage_Player_Ping : public GameMessage
{
public:
	unsigned short ping;

	GameMessage_Player_Ping(): GameMessage(NMS_PLAYER_PING) { } //-V730
	GameMessage_Player_Ping(const unsigned char player, const unsigned short ping): GameMessage(NMS_PLAYER_PING, player), ping(ping){}

	void Serialize(Serializer& ser) const override
    {
        GameMessage::Serialize(ser);
        ser.PushUnsignedShort(ping);
    }

    void Deserialize(Serializer& ser) override
	{
		GameMessage::Deserialize(ser);
        ping = ser.PopUnsignedShort();
	}

	void Run(MessageInterface* callback) override
	{
		//LOG.write("<<< NMS_PLAYER_PING\n");
		GetInterface(callback)->OnGameMessage(*this);
	}
};

///////////////////////////////////////////////////////////////////////////////
/// gehende Player-New-Nachricht
class GameMessage_Player_New : public GameMessage
{
public:
	/// Name des Neuen Spielers
	std::string name;

	GameMessage_Player_New(): GameMessage(NMS_PLAYER_NEW) { }
	GameMessage_Player_New(const unsigned char player, const std::string& name): GameMessage(NMS_PLAYER_NEW, player), name(name)
	{
		LOG.write(">>> NMS_PLAYER_NEW\n");
	}

	void Serialize(Serializer& ser) const override
    {
        GameMessage::Serialize(ser);
        ser.PushString(name);
    }

    void Deserialize(Serializer& ser) override
	{
		GameMessage::Deserialize(ser);
        name = ser.PopString();
    }

	void Run(MessageInterface* callback) override
	{
		LOG.write("<<< NMS_PLAYER_NEW\n");
		GetInterface(callback)->OnGameMessage(*this);
	}
};

///////////////////////////////////////////////////////////////////////////////
/// gehende Player-Ready-Nachricht
class GameMessage_Player_Ready : public GameMessage
{
public:
	/// Ist der Spieler bereit?
	bool ready;

	GameMessage_Player_Ready(): GameMessage(NMS_PLAYER_READY) { } //-V730
	GameMessage_Player_Ready(const unsigned char player, const bool ready): GameMessage(NMS_PLAYER_READY, player), ready(ready)
	{
		LOG.write(">>> NMS_PLAYER_READY\n");
	}

	void Serialize(Serializer& ser) const override
    {
        GameMessage::Serialize(ser);
        ser.PushBool(ready);
    }

    void Deserialize(Serializer& ser) override
	{
		GameMessage::Deserialize(ser);
        ready = ser.PopBool();
	}

	void Run(MessageInterface* callback) override
	{
		LOG.write("<<< NMS_PLAYER_READY\n");
		GetInterface(callback)->OnGameMessage(*this);
	}
};

///////////////////////////////////////////////////////////////////////////////
/// gehende Player-Swap-Nachricht
class GameMessage_Player_Swap : public GameMessage
{
public:
	/// Die beiden Spieler-IDs, die miteinander vertauscht werden sollen
	unsigned char player2;
	GameMessage_Player_Swap(): GameMessage(NMS_PLAYER_SWAP) { } //-V730
	GameMessage_Player_Swap(const unsigned char player, const unsigned char player2): GameMessage(NMS_PLAYER_SWAP, player),  player2(player2)
	{
		LOG.write(">>> NMS_PLAYER_SWAP\n");
	}

	void Serialize(Serializer& ser) const override
    {
        GameMessage::Serialize(ser);
        ser.PushUnsignedChar(player2);
    }

    void Deserialize(Serializer& ser) override
	{
		GameMessage::Deserialize(ser);
        player2 = ser.PopUnsignedChar();
	}

	void Run(MessageInterface* callback) override
	{
		LOG.write("<<< NMS_PLAYER_SWAP\n");
		GetInterface(callback)->OnGameMessage(*this);
	}
};

class GameMessage_Map_Info : public GameMessage
{
public:
	/// Name der Karte
	std::string map_name;
	/// Kartentyp (alte Karte neue Karte, Savegame usw.)
	MapType mt;
    unsigned mapLen, mapCompressedLen;
    unsigned luaLen, luaCompressedLen;

	GameMessage_Map_Info(): GameMessage(NMS_MAP_INFO) { } //-V730
	GameMessage_Map_Info(const std::string& map_name, const MapType mt, const unsigned mapLen, const unsigned mapCompressedLen, const unsigned luaLen, const unsigned luaCompressedLen)
		: GameMessage(NMS_MAP_INFO, 0xFF), map_name(map_name),  mt(mt), mapLen(mapLen), mapCompressedLen(mapCompressedLen), luaLen(luaLen), luaCompressedLen(luaCompressedLen)
	{
		LOG.write(">>> NMS_MAP_INFO\n");
	}

	void Serialize(Serializer& ser) const override
    {
        GameMessage::Serialize(ser);
        ser.PushString(map_name);
        ser.PushUnsignedChar(static_cast<unsigned char>(mt));
        ser.PushUnsignedInt(mapLen);
        ser.PushUnsignedInt(mapCompressedLen);
        ser.PushUnsignedInt(luaLen);
        ser.PushUnsignedInt(luaCompressedLen);
    }

    void Deserialize(Serializer& ser) override
	{
		GameMessage::Deserialize(ser);
        map_name = ser.PopString();
        mt = MapType(ser.PopUnsignedChar());
        mapLen = ser.PopUnsignedInt();
        mapCompressedLen = ser.PopUnsignedInt();
        luaLen = ser.PopUnsignedInt();
        luaCompressedLen = ser.PopUnsignedInt();
    }

	void Run(MessageInterface* callback) override
	{
		LOG.write("<<< NMS_MAP_INFO\n");
		GetInterface(callback)->OnGameMessage(*this);
	}
};

class GameMessage_Map_Data : public GameMessage
{
public:
    bool isMapData;
    /// Offset into map buffer
	unsigned offset;
	/// Kartendaten
	std::vector<char> data;
    /// True for map data, false for luaData

	GameMessage_Map_Data(): GameMessage(NMS_MAP_DATA) { } //-V730
	GameMessage_Map_Data(bool isMapData, const unsigned offset, const char* const data, const unsigned length)
		: GameMessage(NMS_MAP_DATA, 0xFF), isMapData(isMapData), offset(offset), data(data, data + length)
	{
		LOG.write(">>> NMS_MAP_DATA\n");
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

	void Run(MessageInterface* callback) override
	{
		LOG.write("<<< NMS_MAP_DATA\n");
		GetInterface(callback)->OnGameMessage(*this);
	}
};

class GameMessage_Map_Checksum : public GameMessage
{
public:
	/// Checksumme, die vom Client berechnt wurde
	unsigned mapChecksum, luaChecksum;

	GameMessage_Map_Checksum(): GameMessage(NMS_MAP_CHECKSUM) { } //-V730
	GameMessage_Map_Checksum(const unsigned mapChecksum, const unsigned luaChecksum): GameMessage(NMS_MAP_CHECKSUM, 0xFF), mapChecksum(mapChecksum), luaChecksum(luaChecksum)
	{
		LOG.write(">>> NMS_MAP_CHECKSUM\n");
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

	void Run(MessageInterface* callback) override
	{
		LOG.write("<<< NMS_MAP_CHECKSUM\n");
		GetInterface(callback)->OnGameMessage(*this);
	}
};

/// MapChecksum vom Server erfolgreich verifiziert?
class GameMessage_Map_ChecksumOK: public GameMessage
{
public:
	/// Vom Server akzeptiert?
	bool correct;

	GameMessage_Map_ChecksumOK(): GameMessage(NMS_MAP_CHECKSUMOK) { } //-V730
	GameMessage_Map_ChecksumOK(const bool correct): GameMessage(NMS_MAP_CHECKSUMOK, 0xFF), correct(correct){}

	void Serialize(Serializer& ser) const override
    {
        GameMessage::Serialize(ser);
        ser.PushBool(correct);
    }

    void Deserialize(Serializer& ser) override
	{
		GameMessage::Deserialize(ser);
        correct = ser.PopBool();
	}

	void Run(MessageInterface* callback) override
	{
		GetInterface(callback)->OnGameMessage(*this);
	}
};

class GameMessage_GGSChange : public GameMessage
{
public:
	/// GGS-Daten
	GlobalGameSettings ggs;

	GameMessage_GGSChange(): GameMessage(NMS_GGS_CHANGE) {}
	GameMessage_GGSChange(const GlobalGameSettings& ggs): GameMessage(NMS_GGS_CHANGE, 0xFF), ggs(ggs)
	{
		LOG.write(">>> NMS_GGS_CHANGE\n");
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

	void Run(MessageInterface* callback) override
	{
		LOG.write("<<< NMS_GGS_CHANGE\n");
		GetInterface(callback)->OnGameMessage(*this);
	}
};

class GameMessage_RemoveLua: public GameMessage
{
public:
    GameMessage_RemoveLua(): GameMessage(NMS_REMOVE_LUA, 0xFF) {}

    void Run(MessageInterface* callback) override
    {
        GetInterface(callback)->OnGameMessage(*this);
    }
};

class GameMessage_Server_Speed : public GameMessage
{
public:
	unsigned int gf_length; // new speed

	GameMessage_Server_Speed(): GameMessage(NMS_SERVER_SPEED) { } //-V730
	GameMessage_Server_Speed(const unsigned int gf_length): GameMessage(NMS_SERVER_SPEED, 0xFF), gf_length(gf_length)
	{
		LOG.write(">>> NMS_SERVER_SPEED(%d)\n", gf_length);
	}

	void Serialize(Serializer& ser) const override
    {
        GameMessage::Serialize(ser);
        ser.PushUnsignedInt(gf_length);
    }

    void Deserialize(Serializer& ser) override
	{
		GameMessage::Deserialize(ser);
        gf_length = ser.PopUnsignedInt();
	}

	void Run(MessageInterface* callback) override
	{
		LOG.write("<<< NMS_SERVER_SPEED(%d)\n", gf_length);
		GetInterface(callback)->OnGameMessage(*this);
	}
};

class GameMessage_Server_NWFDone : public GameMessage
{
public:
	unsigned int nr; // GF
	unsigned int gf_length; // new speed
	bool first;

	GameMessage_Server_NWFDone(): GameMessage(NMS_SERVER_NWF_DONE) { } //-V730
	GameMessage_Server_NWFDone(const unsigned char player, const unsigned int nr, const unsigned int gf_length, const bool first = false):
        GameMessage(NMS_SERVER_NWF_DONE, player), nr(nr), gf_length(gf_length), first(first)
	{
		LOG.write(">>> NMS_NWF_DONE(%d, %d, %d)\n", nr, gf_length, (first ? 1 : 0));
	}

	void Serialize(Serializer& ser) const override
    {
        GameMessage::Serialize(ser);
        ser.PushUnsignedInt(nr);
        ser.PushUnsignedInt(gf_length);
        ser.PushBool(first);
    }

    void Deserialize(Serializer& ser) override
	{
		GameMessage::Deserialize(ser);
        nr = ser.PopUnsignedInt();
        gf_length = ser.PopUnsignedInt();
        first = ser.PopBool();
	}

	void Run(MessageInterface* callback) override
	{
		LOG.write("<<< NMS_NWF_DONE(%d, %d, %d)\n", nr, gf_length, (first ? 1 : 0));
		GetInterface(callback)->OnGameMessage(*this);
	}
};

class GameMessage_Pause : public GameMessage
{
public:
	/// Pausiert?
    bool paused;
	unsigned int nr; // GF

	GameMessage_Pause(): GameMessage(NMS_PAUSE) { } //-V730
	GameMessage_Pause(const bool paused, const unsigned nr): GameMessage(NMS_PAUSE, 0xFF), paused(paused), nr(nr)
	{
		LOG.write(">>> NMS_PAUSE(%d)\n", paused ? 1 : 0);
	}

	void Serialize(Serializer& ser) const override
    {
        GameMessage::Serialize(ser);
        ser.PushBool(paused);
        ser.PushUnsignedInt(nr);
    }

    void Deserialize(Serializer& ser) override
	{
		GameMessage::Deserialize(ser);
        paused = ser.PopBool();
        nr = ser.PopUnsignedInt();
	}

	void Run(MessageInterface* callback) override
	{
		LOG.write("<<< NMS_PAUSE(%d)\n", paused ? 1 : 0);
		GetInterface(callback)->OnGameMessage(*this);
	}
};

///////////////////////////////////////////////////////////////////////////////
/// ausgehende GetAsyncLog-Nachricht
class GameMessage_GetAsyncLog : public GameMessage
{
public:
	GameMessage_GetAsyncLog(): GameMessage(NMS_GET_ASYNC_LOG) {}
	GameMessage_GetAsyncLog(const unsigned char player): GameMessage(NMS_GET_ASYNC_LOG, player)
	{
		LOG.write(">>> NMS_GET_ASYNC_LOG\n");
	}
	void Run(MessageInterface* callback) override
	{
		LOG.write("<<< NMS_GET_ASYNC_LOG\n");
		GetInterface(callback)->OnGameMessage(*this);
	}
};

///////////////////////////////////////////////////////////////////////////////
/// eingehende SendAsyncLog-Nachricht
class GameMessage_SendAsyncLog : public GameMessage
{
public:
	std::vector<RandomEntry> entries;
    bool last;

	GameMessage_SendAsyncLog(): GameMessage(NMS_SEND_ASYNC_LOG) {} //-V730

	GameMessage_SendAsyncLog(const std::vector<RandomEntry>& async_log, bool last):
        GameMessage(NMS_SEND_ASYNC_LOG, 0xFF), entries(async_log), last(last)
	{
		LOG.write(">>> NMS_SEND_ASYNC_LOG\n");
	}

	void Serialize(Serializer& ser) const override
    {
        GameMessage::Serialize(ser);
        ser.PushBool(last);
        ser.PushUnsignedInt(entries.size());

        for(std::vector<RandomEntry>::const_iterator it = entries.begin(); it != entries.end(); ++it)
        {
            ser.PushUnsignedInt(it->counter);
            ser.PushSignedInt(it->max);
            ser.PushSignedInt(it->rngState);
            ser.PushString(it->src_name);
            ser.PushUnsignedInt(it->src_line);
            ser.PushUnsignedInt(it->obj_id);
        }
    }

    void Deserialize(Serializer& ser) override
	{
		GameMessage::Deserialize(ser);
        last = ser.PopBool();
        unsigned cnt = ser.PopUnsignedInt();
        entries.clear();
        entries.reserve(cnt);

        while (cnt--)
        {
            unsigned counter = ser.PopUnsignedInt();
            int max = ser.PopSignedInt();
            int rngState = ser.PopSignedInt();
            std::string src_name = ser.PopString();
            unsigned src_line = ser.PopUnsignedInt();
            unsigned obj_id = ser.PopUnsignedInt();

            entries.push_back(RandomEntry(counter, max, rngState, src_name, src_line, obj_id));
        }

	}

	void Run(MessageInterface* callback) override
	{
		LOG.write("<<< NMS_SEND_ASYNC_LOG: %u [%s]\n", entries.size(), last ? "last" : "non-last");
		GetInterface(callback)->OnGameMessage(*this);
	}
};

#endif //!GAMEMESSAGES_H_INCLUDED
