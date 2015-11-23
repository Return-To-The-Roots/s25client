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
#include "GameObject.h"
#include "GlobalGameSettings.h"
#include "Random.h"
#include "../libutil/src/Log.h"

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
        GameMessage_Ping(void) : GameMessage(NMS_PING) {}
        GameMessage_Ping(const unsigned char player) : GameMessage(NMS_PING, player) {}

        void Run(MessageInterface* callback)
        {
            //LOG.write("<<< NMS_PING\n");
            GetInterface(callback)->OnNMSPing(*this);
        }
};

///////////////////////////////////////////////////////////////////////////////
/// ausgehende Pong-Nachricht
class GameMessage_Pong : public GameMessage
{
    public:
        GameMessage_Pong(void) : GameMessage(NMS_PONG) { }
        GameMessage_Pong(const unsigned char player) : GameMessage(NMS_PONG, player)
        {
            //LOG.write(">>> NMS_PONG\n");
        }
        void Run(MessageInterface* callback)
        {
            //LOG.write("<<< NMS_PONG\n");
            GetInterface(callback)->OnNMSPong(*this);
        }
};

///////////////////////////////////////////////////////////////////////////////
/// ausgehende Server-Typ-Nachricht
class GameMessage_Server_Type: public GameMessage
{
    public:
        ServerType type;
        std::string version;

    public:
        GameMessage_Server_Type(void) : GameMessage(NMS_SERVER_TYPE) { }
        GameMessage_Server_Type(const ServerType type,
                                const std::string& version) : GameMessage(NMS_SERVER_TYPE, 0xFF)
        {
            LOG.write(">>> NMS_SERVER_Type(%d, %s)\n", boost::underlying_cast<int>(type), version.c_str());

            PushUnsignedShort(boost::underlying_cast<unsigned short>(type));
            PushString(version);
        }
        GameMessage_Server_Type(const unsigned short& type)
            : GameMessage(NMS_SERVER_TYPE, 0xFF)
        {
            LOG.write(">>> NMS_SERVER_Type(%s)\n", (type == 1 ? "true" : "false"));
            PushUnsignedShort(type);
        }
        void Run(MessageInterface* callback)
        {
            type = static_cast<ServerType>(PopUnsignedShort());

            if(GetLength() > sizeof(unsigned short))
            {
                version = PopString();

                LOG.write("<<< NMS_SERVER_Type(%d, %s)\n", boost::underlying_cast<int>(type), version.c_str());
                GetInterface(callback)->OnNMSServerType(*this);
            }
            else
            {
                LOG.write("<<< NMS_SERVER_Type(%s)\n", (type == ServerType::DIRECT ? "true" : "false"));
                GetInterface(callback)->OnNMSServerType(*this);
            }
        }
};


class GameMessage_Server_TypeOK: public GameMessage
{
    public:
        /// Vom Server akzeptiert?
        unsigned err_code;

    public:
        GameMessage_Server_TypeOK(void) : GameMessage(NMS_SERVER_TYPEOK) { }
        GameMessage_Server_TypeOK(const unsigned err_code)
            : GameMessage(NMS_SERVER_TYPEOK, 0xFF), err_code(err_code)
        {
            PushUnsignedInt(err_code);
        }

        void Run(MessageInterface* callback)
        {
            err_code = PopUnsignedInt();
            GetInterface(callback)->OnNMSServerTypeOK(*this);
        }
};


///////////////////////////////////////////////////////////////////////////////
/// ein/ausgehende Server-Password-Nachricht
class GameMessage_Server_Password : public GameMessage
{
    public:
        std::string password;

    public:
        GameMessage_Server_Password(void) : GameMessage(NMS_SERVER_PASSWORD) { }
        GameMessage_Server_Password(const std::string& password)
            : GameMessage(NMS_SERVER_PASSWORD, 0xFF)
        {
            LOG.write(">>> NMS_SERVER_PASSWORD(%s)\n", "********");

            PushString(password);
        }
        void Run(MessageInterface* callback)
        {
            password = PopString();

            LOG.write("<<< NMS_SERVER_PASSWORD(%s)\n", "********");
            GetInterface(callback)->OnNMSServerPassword(*this);
        }
};

///////////////////////////////////////////////////////////////////////////////
/// ausgehende Server-Name-Nachricht
class GameMessage_Server_Name : public GameMessage
{
    public:
        std::string name;

    public:
        GameMessage_Server_Name(void) : GameMessage(NMS_SERVER_NAME) { }
        GameMessage_Server_Name(const std::string& name)
            : GameMessage(NMS_SERVER_NAME, 0xFF)
        {
            LOG.write(">>> NMS_SERVER_NAME(%s)\n", name.c_str());

            PushString(name);
        }
        void Run(MessageInterface* callback)
        {
            name = PopString();

            LOG.write("<<< NMS_SERVER_NAME(%s)\n", name.c_str());
            GetInterface(callback)->OnNMSServerName(*this);
        }
};

///////////////////////////////////////////////////////////////////////////////
/// eingehende Server-Start-Nachricht
class GameMessage_Server_Start : public GameMessage
{
    public:
        unsigned int random_init;
        unsigned int nwf_length;

    public:
        GameMessage_Server_Start(void) : GameMessage(NMS_SERVER_START) { }
        GameMessage_Server_Start(const unsigned random_init,
                                 const unsigned nwf_length) : GameMessage(NMS_SERVER_START, 0xFF)
        {
            LOG.write(">>> NMS_SERVER_START(%d, %d)\n", random_init, nwf_length);

            PushUnsignedInt(random_init);
            PushUnsignedInt(nwf_length);
        }
        void Run(MessageInterface* callback)
        {
            random_init = PopUnsignedInt();
            nwf_length = PopUnsignedInt();

            LOG.write("<<< NMS_SERVER_START(%d, %d)\n", random_init, nwf_length);
            GetInterface(callback)->OnNMSServerStart(*this);
        }
};

///////////////////////////////////////////////////////////////////////////////
/// eingehende Server-Countdown-Nachricht
class GameMessage_Server_Countdown : public GameMessage
{
    public:
        int countdown;

    public:
        GameMessage_Server_Countdown(void) : GameMessage(NMS_SERVER_COUNTDOWN) { }
        GameMessage_Server_Countdown(int countdown) : GameMessage(NMS_SERVER_COUNTDOWN, 0xFF)
        {
            PushUnsignedInt(countdown);

            LOG.write(">>> NMS_SERVER_COUNTDOWN(%d)\n", countdown);
        }
        void Run(MessageInterface* callback)
        {
            countdown = PopUnsignedInt();

            LOG.write("<<< NMS_SERVER_COUNTDOWN(%d)\n", countdown);
            GetInterface(callback)->OnNMSServerCountdown(*this);
        }
};

///////////////////////////////////////////////////////////////////////////////
/// eingehende Server-CancelCountdown-Nachricht
class GameMessage_Server_CancelCountdown : public GameMessage
{
    public:
        GameMessage_Server_CancelCountdown(void) : GameMessage(NMS_SERVER_CANCELCOUNTDOWN) { }
        GameMessage_Server_CancelCountdown(bool reserved) : GameMessage(NMS_SERVER_CANCELCOUNTDOWN, 0xFF)
        {
            LOG.write(">>> NMS_SERVER_CANCELCOUNTDOWN\n");
        }
        void Run(MessageInterface* callback)
        {
            LOG.write("<<< NMS_SERVER_CANCELCOUNTDOWN\n");
            GetInterface(callback)->OnNMSServerCancelCountdown(*this);
        }
};

///////////////////////////////////////////////////////////////////////////////
/// ein/ausgehende Server-Chat-Nachricht
class GameMessage_Server_Chat : public GameMessage
{
    public:
        ChatDestination destination;
        std::string text;

    public:
        GameMessage_Server_Chat(void) : GameMessage(NMS_SERVER_CHAT) { }
        GameMessage_Server_Chat(const unsigned char player,
                                const ChatDestination destination, const std::string& text) : GameMessage(NMS_SERVER_CHAT, player)
        {
            LOG.write(">>> NMS_SERVER_CHAT(%d, %s)\n", destination, text.c_str());

            PushUnsignedChar(static_cast<unsigned char>(destination));
            PushString(text);
        }

        void Run(MessageInterface* callback)
        {
            destination = ChatDestination(PopUnsignedChar());
            text = PopString();

            LOG.write("<<< NMS_SERVER_CHAT(%d, %s)\n", destination, text.c_str());
            GetInterface(callback)->OnNMSServerChat(*this);
        }
};

///////////////////////////////////////////////////////////////////////////////
/// eingehende Server-Async-Nachricht
class GameMessage_Server_Async : public GameMessage
{
    public:
        std::vector<int> checksums;

    public:
        GameMessage_Server_Async(void) : GameMessage(NMS_SERVER_ASYNC) { }
        GameMessage_Server_Async(const std::vector<int>& checksums)
            : GameMessage(NMS_SERVER_ASYNC, 0xFF)
        {
            LOG.write(">>> NMS_SERVER_ASYNC(%d)\n", checksums.size());

            PushUnsignedInt(unsigned(checksums.size()));
            for(unsigned int i = 0; i < checksums.size(); ++i)
                PushSignedInt(checksums[i]);
        }
        void Run(MessageInterface* callback)
        {
            unsigned size = PopUnsignedInt();
            checksums.resize(size);
            for(unsigned int i = 0; i < size; ++i)
                checksums[i] = PopSignedInt();

            LOG.write("<<< NMS_SERVER_ASYNC(%d)\n", checksums.size());
            GetInterface(callback)->OnNMSServerAsync(*this);
        }
};

///////////////////////////////////////////////////////////////////////////////
/// eingehende Player-ID-Nachricht
class GameMessage_Player_Id : public GameMessage
{
    public:
        unsigned int playerid;

    public:
        GameMessage_Player_Id(void) : GameMessage(NMS_PLAYER_ID) { }
        GameMessage_Player_Id(const unsigned int playerid)
            : GameMessage(NMS_PLAYER_ID, 0xFF)
        {
            LOG.write(">>> NMS_PLAYER_ID(%d)\n", playerid);

            PushUnsignedInt(playerid);
        }
        void Run(MessageInterface* callback)
        {
            playerid = PopUnsignedInt();

            LOG.write("<<< NMS_PLAYER_ID(%d)\n", playerid);
            GetInterface(callback)->OnNMSPlayerId(*this);
        }
};

///////////////////////////////////////////////////////////////////////////////
/// ausgehende Player-Name-Nachricht
class GameMessage_Player_Name : public GameMessage
{
    public:
        /// Name des neuen Spielers
        std::string playername;

    public:
        GameMessage_Player_Name(void) : GameMessage(NMS_PLAYER_NAME) { }
        GameMessage_Player_Name(const std::string& playername)
            : GameMessage(NMS_PLAYER_NAME, 0xFF)
        {
            LOG.write(">>> NMS_PLAYER_NAME(%s)\n", playername.c_str());

            PushString(playername);
        }
        void Run(MessageInterface* callback)
        {
            playername = PopString();

            LOG.write("<<< NMS_PLAYER_NAME(%s)\n", playername.c_str());
            GetInterface(callback)->OnNMSPlayerName(*this);
        }
};

///////////////////////////////////////////////////////////////////////////////
/// eingehende Player-List-Nachricht
class GameMessage_Player_List : public GameMessage
{
    public:
        GameServerPlayerList gpl;

    public:
        GameMessage_Player_List(void) : GameMessage(NMS_PLAYER_LIST) { }
        GameMessage_Player_List(const GameServerPlayerList& gpl)
            : GameMessage(NMS_PLAYER_LIST, 0xFF)
        {
            LOG.write(">>> NMS_PLAYER_LIST(%d)\n", gpl.getCount());

            gpl.serialize(this);
        }
        void Run(MessageInterface* callback)
        {
            gpl.deserialize(this);

            LOG.write("<<< NMS_PLAYER_LIST(%d)\n", gpl.getCount());
            for(unsigned int i = 0; i < gpl.getCount(); ++i)
            {
                const GamePlayerInfo* playerInfo = gpl.getElement(i);
                LOG.write("    %d: %s %d %d %d %d %d %d %s\n", i, playerInfo->name.c_str(), playerInfo->ps, playerInfo->rating, playerInfo->ping, playerInfo->nation, playerInfo->color, playerInfo->team, (playerInfo->ready ? "true" : "false") );
            }
            GetInterface(callback)->OnNMSPlayerList(*this);
        }
};

///////////////////////////////////////////////////////////////////////////////
/// gehende -Nachricht
class GameMessage_Player_Toggle_State : public GameMessage
{
    public:
        GameMessage_Player_Toggle_State(void) : GameMessage(NMS_PLAYER_TOGGLESTATE) { }
        GameMessage_Player_Toggle_State(const unsigned char player)
            : GameMessage(NMS_PLAYER_TOGGLESTATE, player)
        {
            LOG.write(">>> NMS_PLAYER_TOGGLESTATE(%d)\n", player);
        }
        void Run(MessageInterface* callback)
        {
            LOG.write("<<< NMS_PLAYER_TOGGLESTATE(%d)\n", player);
            GetInterface(callback)->OnNMSPlayerToggleState(*this);
        }
};

///////////////////////////////////////////////////////////////////////////////
/// gehende -Nachricht
class GameMessage_Player_Toggle_Nation : public GameMessage
{
    public:
        /// Das zu setzende Volk
        Nation nation;

    public:
        GameMessage_Player_Toggle_Nation(void) : GameMessage(NMS_PLAYER_TOGGLENATION) { }
        GameMessage_Player_Toggle_Nation(const unsigned char player, const Nation nation)
            : GameMessage(NMS_PLAYER_TOGGLENATION, player), nation(nation)
        {
            PushUnsignedChar(static_cast<unsigned char>(nation));
            LOG.write(">>> NMS_PLAYER_TOGGLENATION\n");
        }
        void Run(MessageInterface* callback)
        {
            nation = Nation(PopUnsignedChar());

            LOG.write("<<< NMS_PLAYER_TOGGLENATION\n");
            GetInterface(callback)->OnNMSPlayerToggleNation(*this);
        }
};

///////////////////////////////////////////////////////////////////////////////
/// gehende Player-Toggle-Team-Nachricht
class GameMessage_Player_Toggle_Team : public GameMessage
{
    public:
        /// Das zu setzende Team
        Team team;
    public:
        GameMessage_Player_Toggle_Team(void) : GameMessage(NMS_PLAYER_TOGGLETEAM) { }
        GameMessage_Player_Toggle_Team(const unsigned char player, const Team team)
            : GameMessage(NMS_PLAYER_TOGGLETEAM, player), team(team)
        {
            PushUnsignedChar(static_cast<unsigned char>(team));
            LOG.write(">>> NMS_PLAYER_TOGGLETEAM\n");
        }
        void Run(MessageInterface* callback)
        {
            team = Team(PopUnsignedChar());

            LOG.write("<<< NMS_PLAYER_TOGGLETEAM\n");
            GetInterface(callback)->OnNMSPlayerToggleTeam(*this);
        }
};

///////////////////////////////////////////////////////////////////////////////
/// gehende Player-Toggle-Color-Nachricht
class GameMessage_Player_Toggle_Color : public GameMessage
{
    public:
        /// Das zu setzende Team
        unsigned char color;
    public:
        GameMessage_Player_Toggle_Color(void) : GameMessage(NMS_PLAYER_TOGGLECOLOR) { }
        GameMessage_Player_Toggle_Color(const unsigned char player, const unsigned char color)
            : GameMessage(NMS_PLAYER_TOGGLECOLOR, player), color(color)
        {
            PushUnsignedChar(color);

            LOG.write(">>> NMS_PLAYER_TOGGLECOLOR\n");
        }
        void Run(MessageInterface* callback)
        {
            color = PopUnsignedChar();

            LOG.write("<<< NMS_PLAYER_TOGGLECOLOR\n");
            GetInterface(callback)->OnNMSPlayerToggleColor(*this);
        }
};

///////////////////////////////////////////////////////////////////////////////
/// gehende Player-Kicked-Nachricht
class GameMessage_Player_Kicked : public GameMessage
{
    public:
        unsigned char cause;
        unsigned short param;

    public:
        GameMessage_Player_Kicked(void) : GameMessage(NMS_PLAYER_KICKED) { }
        GameMessage_Player_Kicked(const unsigned char player, const unsigned char cause, const unsigned short param)
            : GameMessage(NMS_PLAYER_KICKED, player),
              cause(cause), param(param)
        {
            PushUnsignedChar(cause);
            PushUnsignedShort(param);
        }

        void Run(MessageInterface* callback)
        {
            cause = PopUnsignedChar();
            param = PopUnsignedShort();

            LOG.write("<<< NMS_PLAYER_KICKED\n");
            GetInterface(callback)->OnNMSPlayerKicked(*this);
        }
};

///////////////////////////////////////////////////////////////////////////////
/// gehende Player-Ping-Nachricht
class GameMessage_Player_Ping : public GameMessage
{
    public:
        unsigned short ping;
    public:
        GameMessage_Player_Ping(void) : GameMessage(NMS_PLAYER_PING) { }
        GameMessage_Player_Ping(const unsigned char player, const unsigned short ping)
            : GameMessage(NMS_PLAYER_PING, player), ping(ping)
        {
            PushUnsignedShort(ping);
            //LOG.write(">>> NMS_PLAYER_PING\n");
        }
        void Run(MessageInterface* callback)
        {
            ping = PopUnsignedShort();
            //LOG.write("<<< NMS_PLAYER_PING\n");
            GetInterface(callback)->OnNMSPlayerPing(*this);
        }
};

///////////////////////////////////////////////////////////////////////////////
/// gehende Player-New-Nachricht
class GameMessage_Player_New : public GameMessage
{
    public:
        /// Name des Neuen Spielers
        std::string name;
    public:
        GameMessage_Player_New(void) : GameMessage(NMS_PLAYER_NEW) { }
        GameMessage_Player_New(const unsigned char player, const std::string& name)
            : GameMessage(NMS_PLAYER_NEW, player)
        {
            PushString(name);
            LOG.write(">>> NMS_PLAYER_NEW\n");
        }
        void Run(MessageInterface* callback)
        {
            name = PopString();

            LOG.write("<<< NMS_PLAYER_NEW\n");
            GetInterface(callback)->OnNMSPlayerNew(*this);
        }
};

///////////////////////////////////////////////////////////////////////////////
/// gehende Player-Ready-Nachricht
class GameMessage_Player_Ready : public GameMessage
{
    public:
        /// Ist der Spieler bereit?
        bool ready;
    public:
        GameMessage_Player_Ready(void) : GameMessage(NMS_PLAYER_READY) { }
        GameMessage_Player_Ready(const unsigned char player, const bool ready)
            : GameMessage(NMS_PLAYER_READY, player), ready(ready)
        {
            PushBool(ready);
            LOG.write(">>> NMS_PLAYER_READY\n");
        }
        void Run(MessageInterface* callback)
        {
            ready = PopBool();
            LOG.write("<<< NMS_PLAYER_READY\n");
            GetInterface(callback)->OnNMSPlayerReady(*this);
        }
};

///////////////////////////////////////////////////////////////////////////////
/// gehende Player-Swap-Nachricht
class GameMessage_Player_Swap : public GameMessage
{
    public:
        /// Die beiden Spieler-IDs, die miteinander vertauscht werden sollen
        unsigned char player2;
    public:
        GameMessage_Player_Swap(void) : GameMessage(NMS_PLAYER_SWAP) { }
        GameMessage_Player_Swap(const unsigned char player, const unsigned char player2)
            : GameMessage(NMS_PLAYER_SWAP, player),  player2(player2)
        {
            PushUnsignedChar(player2);

            LOG.write(">>> NMS_PLAYER_SWAP\n");
        }
        void Run(MessageInterface* callback)
        {
            player2 = PopUnsignedChar();

            LOG.write("<<< NMS_PLAYER_SWAP\n");
            GetInterface(callback)->OnNMSPlayerSwap(*this);
        }
};

class GameMessage_Map_Info : public GameMessage
{
    public:
        /// Name der Karte
        std::string map_name;
        /// Kartentyp (alte Karte neue Karte, Savegame usw.)
        MapType mt;
        /// Größe der Zip-komprimierten Date
        unsigned ziplength;
        /// Größe der dekomprimierten Daten
        unsigned normal_length;
        /// LUA script
        std::string script;

    public:
        GameMessage_Map_Info(void) : GameMessage(NMS_MAP_INFO) { }
        GameMessage_Map_Info(const std::string& map_name, const MapType mt,
                             const unsigned ziplength, const unsigned normal_length, const std::string& script)
            : GameMessage(NMS_MAP_INFO, 0xFF), map_name(map_name),  mt(mt), ziplength(ziplength),
              normal_length(normal_length), script(script)
        {
            PushString(map_name);
            PushUnsignedChar(static_cast<unsigned char>(mt));
            PushUnsignedInt(ziplength);
            PushUnsignedInt(normal_length);
            PushString(script);

            LOG.write(">>> NMS_MAP_INFO\n");
        }
        void Run(MessageInterface* callback)
        {
            map_name = PopString();
            mt = MapType(PopUnsignedChar());
            ziplength = PopUnsignedInt();
            normal_length = PopUnsignedInt();
            script = PopString();

            LOG.write("<<< NMS_MAP_INFO\n");
            GetInterface(callback)->OnNMSMapInfo(*this);
        }
};

class GameMessage_Map_Data : public GameMessage
{
    public:
        /// Offset into map buffer
        unsigned offset;
        /// Kartendaten
        const unsigned char* map_data;

    public:
        GameMessage_Map_Data(void) : GameMessage(NMS_MAP_DATA) { }
        GameMessage_Map_Data(const unsigned offset, const unsigned char* const map_data, const unsigned length)
            : GameMessage(NMS_MAP_DATA, 0xFF)
        {
            PushUnsignedInt(offset);
            PushRawData(map_data, length);

            LOG.write(">>> NMS_MAP_DATA\n");
        }

        unsigned GetDataLength() const
        {
            return GetNetLength() - sizeof(offset);
        }

        void Run(MessageInterface* callback)
        {
            offset = PopUnsignedInt();
            map_data = GetData() + (GetLength() - GetDataLength());

            LOG.write("<<< NMS_MAP_DATA\n");
            GetInterface(callback)->OnNMSMapData(*this);
        }
};

class GameMessage_Map_Checksum : public GameMessage
{
    public:
        /// Checksumme, die vom Client berechnt wurde
        unsigned checksum;

    public:
        GameMessage_Map_Checksum(void) : GameMessage(NMS_MAP_CHECKSUM) { }
        GameMessage_Map_Checksum(const unsigned checksum)
            : GameMessage(NMS_MAP_CHECKSUM, 0xFF)
        {
            PushUnsignedInt(checksum);

            LOG.write(">>> NMS_MAP_CHECKSUM\n");
        }
        void Run(MessageInterface* callback)
        {
            checksum = PopUnsignedInt();
            LOG.write("<<< NMS_MAP_CHECKSUM\n");
            GetInterface(callback)->OnNMSMapChecksum(*this);
        }
};

/// MapChecksum vom Server erfolgreich verifiziert?
class GameMessage_Map_ChecksumOK: public GameMessage
{
    public:
        /// Vom Server akzeptiert?
        bool correct;

    public:
        GameMessage_Map_ChecksumOK(void) : GameMessage(NMS_MAP_CHECKSUMOK) { }
        GameMessage_Map_ChecksumOK(const bool correct)
            : GameMessage(NMS_MAP_CHECKSUMOK, 0xFF), correct(correct)
        {
            PushBool(correct);
        }

        void Run(MessageInterface* callback)
        {
            correct = PopBool();
            GetInterface(callback)->OnNMSMapChecksumOK(*this);
        }
};


class GameMessage_GGSChange : public GameMessage
{
    public:
        /// GGS-Daten
        GlobalGameSettings ggs;

    public:
        GameMessage_GGSChange(void) : GameMessage(NMS_GGS_CHANGE) { }
        GameMessage_GGSChange(const GlobalGameSettings& ggs)
            : GameMessage(NMS_GGS_CHANGE, player)
        {
            ggs.Serialize(this);

            LOG.write(">>> NMS_GGS_CHANGE\n");
        }
        void Run(MessageInterface* callback)
        {
            ggs.Deserialize(this);

            LOG.write("<<< NMS_GGS_CHANGE\n");
            GetInterface(callback)->OnNMSGGSChange(*this);
        }
};

class GameMessage_Server_Speed : public GameMessage
{
    public:
        unsigned int gf_length; // new speed

    public:
        GameMessage_Server_Speed(void) : GameMessage(NMS_SERVER_SPEED) { }
        GameMessage_Server_Speed(const unsigned int gf_length) : GameMessage(NMS_SERVER_SPEED, player)
        {
            PushUnsignedInt(gf_length);
            LOG.write(">>> NMS_SERVER_SPEED(%d)\n", gf_length);
        }

        void Run(MessageInterface* callback)
        {
            gf_length = PopUnsignedInt();

            LOG.write("<<< NMS_SERVER_SPEED(%d)\n", gf_length);
            GetInterface(callback)->OnNMSServerSpeed(*this);
        }
};

class GameMessage_Server_NWFDone : public GameMessage
{
    public:
        unsigned int nr; // GF
        unsigned int gf_length; // new speed
        bool first;

    public:
        GameMessage_Server_NWFDone(void) : GameMessage(NMS_SERVER_NWF_DONE) { }
        GameMessage_Server_NWFDone(const unsigned char player, const unsigned int nr, const unsigned int gf_length, const bool first = false) : GameMessage(NMS_SERVER_NWF_DONE, player)
        {
            PushUnsignedInt(nr);
            PushUnsignedInt(gf_length);
            PushBool(first);
            LOG.write(">>> NMS_NWF_DONE(%d, %d, %d)\n", nr, gf_length, (first ? 1 : 0));
        }

        void Run(MessageInterface* callback)
        {
            nr = PopUnsignedInt();
            gf_length = PopUnsignedInt();
            first = PopBool();

            LOG.write("<<< NMS_NWF_DONE(%d, %d, %d)\n", nr, gf_length, (first ? 1 : 0));
            GetInterface(callback)->OnNMSServerDone(*this);
        }
};

class GameMessage_Pause : public GameMessage
{
    public:
        /// Pausiert?
		unsigned int nr; // GF
        bool paused;

    public:
        GameMessage_Pause(void) : GameMessage(NMS_PAUSE) { }
        GameMessage_Pause(const bool paused, const unsigned nr)
            : GameMessage(NMS_PAUSE, 0xFF)
        {
            PushBool(paused);
			PushUnsignedInt(nr);
            LOG.write(">>> NMS_PAUSE(%d)\n", paused ? 1 : 0);
        }
        void Run(MessageInterface* callback)
        {
            paused = PopBool();
			nr = PopUnsignedInt();

            LOG.write("<<< NMS_PAUSE(%d)\n", paused ? 1 : 0);
            GetInterface(callback)->OnNMSPause(*this);
        }
};

///////////////////////////////////////////////////////////////////////////////
/// ausgehende GetAsyncLog-Nachricht
class GameMessage_GetAsyncLog : public GameMessage
{
    public:
        GameMessage_GetAsyncLog() : GameMessage(NMS_GET_ASYNC_LOG) {}
        GameMessage_GetAsyncLog(const unsigned char player) : GameMessage(NMS_GET_ASYNC_LOG, player)
        {
            LOG.write(">>> NMS_GET_ASYNC_LOG\n");
        }
        void Run(MessageInterface* callback)
        {
            LOG.write("<<< NMS_GET_ASYNC_LOG\n");
            GetInterface(callback)->OnNMSGetAsyncLog(*this);
        }
};

///////////////////////////////////////////////////////////////////////////////
/// eingehende SendAsyncLog-Nachricht
class GameMessage_SendAsyncLog : public GameMessage
{
        std::vector<RandomEntry> recved_log;

    public:
        GameMessage_SendAsyncLog() : GameMessage(NMS_SEND_ASYNC_LOG) {}

        GameMessage_SendAsyncLog(const std::vector<RandomEntry>& async_log, bool last) : GameMessage(NMS_SEND_ASYNC_LOG, 0xFF)
        {
            PushBool(last);
            PushUnsignedInt(async_log.size());

            for(std::vector<RandomEntry>::const_iterator it = async_log.begin(); it != async_log.end(); ++it)
            {
                PushUnsignedInt(it->counter);
                PushSignedInt(it->max);
                PushSignedInt(it->value);
                PushString(it->src_name);
                PushUnsignedInt(it->src_line);
                PushUnsignedInt(it->obj_id);
            }

            LOG.write(">>> NMS_SEND_ASYNC_LOG\n");
        }
        void Run(MessageInterface* callback)
        {
            bool last =  PopBool();
            unsigned int cnt = PopUnsignedInt();
            unsigned counter;
            int max;
            int value;
            std::string src_name;
            unsigned int src_line;
            unsigned obj_id;

            LOG.write("<<< NMS_SEND_ASYNC_LOG: %u [%s]\n", cnt, last ? "last" : "non-last");

            recved_log.clear();

            while (cnt--)
            {
                counter = PopUnsignedInt();
                max = PopSignedInt();
                value = PopSignedInt();
                src_name = PopString();
                src_line = PopUnsignedInt();
                obj_id = PopUnsignedInt();

                recved_log.push_back(RandomEntry(counter, max, value, src_name, src_line, obj_id));
            }

            GetInterface(callback)->OnNMSSendAsyncLog(*this, recved_log, last);
        }
};

#endif //!GAMEMESSAGES_H_INCLUDED
