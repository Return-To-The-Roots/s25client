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

#ifndef GAMESERVER_H_
#define GAMESERVER_H_

#pragma once

#include "Singleton.h"
#include "Socket.h"

#include "GameMessageInterface.h"

#include "GlobalGameSettings.h"
#include "GamePlayerList.h"

#include "ai/AIEventManager.h"
#include "FramesInfo.h"
#include "helpers/Deleter.h"
#include <LANDiscoveryService.h>
#include <boost/interprocess/smart_ptr/unique_ptr.hpp>

class GameServerPlayer;
class GlobalGameSettings;
struct CreateServerInfo;
class GameMessage;
class AIBase;

class GameServer : public Singleton<GameServer, SingletonPolicies::WithLongevity>, public GameMessageInterface
{
    public:
        BOOST_STATIC_CONSTEXPR unsigned Longevity = 6;

        GameServer(void);
        ~GameServer(void);

        /// "Versucht" den Server zu starten (muss ggf. erst um Erlaubnis beim LobbyClient fragen)
        bool TryToStart(const CreateServerInfo& csi, const std::string& map_path, const MapType map_type);
        /// Startet den Server, muss vorher TryToStart aufgerufen werden!
        bool Start();

        void Run(void);
        void Stop(void);

        bool StartGame(void);
        bool StartCountdown(void);
        void CancelCountdown(void);

        bool TogglePause();
		bool IsPaused(){return framesinfo.isPaused;}

        void TogglePlayerNation(unsigned char client);
        void TogglePlayerTeam(unsigned char client);
        void TogglePlayerColor(unsigned char client);
        void TogglePlayerState(unsigned char client);
        void ChangeGlobalGameSettings(const GlobalGameSettings& ggs);

        /// Lässt einen Spieler wechseln (nur zu Debugzwecken)
        void ChangePlayer(const unsigned char old_id, const unsigned char new_id);

        /// Tauscht Spieler(positionen) bei Savegames in dskHostGame
        void SwapPlayer(const unsigned char player1, const unsigned char player2);

        void AIChat(const GameMessage& msg) { SendToAll(msg); }

        std::string GetGameName() const { return serverconfig.gamename; }
        bool HasPwd() const { return !serverconfig.password.empty(); }
        unsigned short GetPort() const { return serverconfig.port; }

    protected:

        void SendToAll(const GameMessage& msg);
        void KickPlayer(unsigned char playerid, unsigned char cause, unsigned short param);
        void KickPlayer(NS_PlayerKicked npk);

        void ClientWatchDog(void);

        void WaitForClients(void);
        void FillPlayerQueues(void);

        /// Sendet ein NC-Paket ohne Befehle
        void SendNothingNC(const unsigned int& id);

        /// Generiert einen KI-Namen
        void SetAIName(const unsigned player_id);

        unsigned GetFilledSlots() const;
        /// Notifies listeners (e.g. Lobby) that the game status has changed (e.g player count)
        void AnnounceStatusChange();
    private:
        void OnNMSPong(const GameMessage_Pong& msg);
        void OnNMSServerType(const GameMessage_Server_Type& msg);
        void OnNMSServerPassword(const GameMessage_Server_Password& msg);
        void OnNMSServerChat(const GameMessage_Server_Chat& msg);
        void OnNMSPlayerName(const GameMessage_Player_Name& msg);
        void OnNMSPlayerToggleNation(const GameMessage_Player_Toggle_Nation& msg);
        void OnNMSPlayerToggleTeam(const GameMessage_Player_Toggle_Team& msg);
        void OnNMSPlayerToggleColor(const GameMessage_Player_Toggle_Color& msg);
        void OnNMSPlayerReady(const GameMessage_Player_Ready& msg);
        void OnNMSMapChecksum(const GameMessage_Map_Checksum& msg);
        void OnNMSGameCommand(const GameMessage_GameCommand& msg);
        void OnNMSServerSpeed(const GameMessage_Server_Speed& msg);

        void OnNMSSendAsyncLog(const GameMessage_SendAsyncLog& msg, const std::vector<RandomEntry>& his, bool last);

        /// Handles advancing of GFs, actions of AI and potentially the NWF
        void ExecuteGameFrame();

        void RunGF( bool isNWF );

        void ExecuteNWF(const unsigned currentTime);

        void CheckAndKickLaggingPlayer(const unsigned char playerIdx);

        unsigned char GetLaggingPlayer() const;

    private:
        enum ServerState
        {
            SS_STOPPED = 0,
            SS_CONFIG,
            SS_GAME
        } status;

        FramesInfo framesinfo;

        class ServerConfig
        {
            public:

                ServerConfig();
                void Clear();

                ServerType servertype;
                unsigned char playercount;
                std::string gamename;
                std::string password;
                std::string mapname;
                unsigned short port;
                bool ipv6;
                bool use_upnp;
        } serverconfig;

        class MapInfo
        {
            public:

                MapInfo();
                void Clear();

                unsigned int ziplength;
                unsigned int length;
                unsigned int checksum;
                std::string name;
                std::string title; // Titel der Karte (nicht der Dateiname!)
                boost::interprocess::unique_ptr<unsigned char, Deleter<unsigned char[]> > zipdata;
                MapType map_type;
                std::string script;
        } mapinfo;

        Socket serversocket;
        GameServerPlayerList players;
        GlobalGameSettings ggs_;

        /// der Spielstartcountdown
        class CountDown
        {
            public:
                CountDown();
                void Clear(int time = 2);

                bool do_countdown;
                int countdown;
                unsigned int lasttime;
        } countdown;

        /// Alle KI-Spieler und ihre Daten (NULL, falls ein solcher Spieler nicht existiert)
        std::vector<AIBase*> ai_players;

        /// AsyncLogs of two async players
        int async_player1, async_player2;
        bool async_player1_done, async_player2_done;
        std::vector<RandomEntry> async_player1_log, async_player2_log;

        LANDiscoveryService lanAnnouncer;

    public:
        AIBase* GetAIPlayer(unsigned playerID) { return ai_players[playerID]; }
        void SendAIEvent(AIEvent::Base* ev, unsigned receiver);
		unsigned int skiptogf;

};

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#define GAMESERVER GameServer::inst()

#endif
