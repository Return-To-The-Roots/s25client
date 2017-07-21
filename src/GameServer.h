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
#include "GameServerInterface.h"
#include "GameMessageInterface.h"
#include "GlobalGameSettings.h"
#include "gameTypes/MapInfo.h"
#include "gameTypes/ServerType.h"
#include "FramesInfo.h"
#include "Random.h"
#include "helpers/Deleter.h"
#include "libutil/src/LANDiscoveryService.h"
#include <boost/interprocess/smart_ptr/unique_ptr.hpp>
#include <vector>

class AIBase;
struct CreateServerInfo;
class GameMessage;
class GameMessage_GameCommand;
class GameServerPlayer;
namespace AIEvent { class Base; }

class GameServer : public Singleton<GameServer, SingletonPolicies::WithLongevity>, public GameMessageInterface, private GameServerInterface
{
    public:
        BOOST_STATIC_CONSTEXPR unsigned Longevity = 6;

        GameServer();
        ~GameServer() override;

        /// "Versucht" den Server zu starten (muss ggf. erst um Erlaubnis beim LobbyClient fragen)
        bool TryToStart(const CreateServerInfo& csi, const std::string& map_path, const MapType map_type);
        /// Startet den Server, muss vorher TryToStart aufgerufen werden!
        bool Start();

        void Run();
        void Stop();

        bool StartGame();
        bool StartCountdown();
        void CancelCountdown();

        void SetPaused(bool paused);
		bool IsPaused(){return framesinfo.isPaused;}

        void ToggleAINation(unsigned char playerId);
        void ToggleAITeam(unsigned char playerId);
        void ToggleAIColor(unsigned char playerId);
        void TogglePlayerState(unsigned char playerId);
        void ChangeGlobalGameSettings(const GlobalGameSettings& ggs) override;
        /// Removes the lua script for the currently loaded map (only valid in config mode)
        void RemoveLuaScript();

        /// Tauscht Spieler(positionen) bei Savegames in dskHostGame
        void SwapPlayer(const unsigned char player1, const unsigned char player2);

        void AIChat(const GameMessage& msg) { SendToAll(msg); }

        std::string GetGameName() const { return config.gamename; }
        bool HasPwd() const { return !config.password.empty(); }
        unsigned short GetPort() const { return config.port; }
        unsigned GetMaxPlayerCount() const override { return config.playercount; }
        bool IsRunning() const override { return status != SS_STOPPED; }

        const GlobalGameSettings& GetGGS() const override { return ggs_; }

        GameServerInterface& GetInterface() { return *this; }

private:

        /// Lässt einen Spieler wechseln (nur zu Debugzwecken)
        void ChangePlayer(const unsigned char old_id, const unsigned char new_id);

        void SendToAll(const GameMessage& msg) override;
        void KickPlayer(unsigned char playerId, unsigned char cause, unsigned short param);
        void KickPlayer(unsigned playerIdx) override;

        void ClientWatchDog();

        void WaitForClients();
        void FillPlayerQueues();

        /// Sendet ein NC-Paket ohne Befehle
        void SendNothingNC(const unsigned& id);

        unsigned GetFilledSlots() const;
        /// Notifies listeners (e.g. Lobby) that the game status has changed (e.g player count)
        void AnnounceStatusChange() override;

        void OnGameMessage(const GameMessage_Pong& msg) override;
        void OnGameMessage(const GameMessage_Server_Type& msg) override;
        void OnGameMessage(const GameMessage_Server_Password& msg) override;
        void OnGameMessage(const GameMessage_Server_Chat& msg) override;
        void OnGameMessage(const GameMessage_System_Chat& msg) override;
        void OnGameMessage(const GameMessage_Player_Name& msg) override;
        void OnGameMessage(const GameMessage_Player_Set_Nation& msg) override;
        void OnGameMessage(const GameMessage_Player_Set_Team& msg) override;
        void OnGameMessage(const GameMessage_Player_Set_Color& msg) override;
        void OnGameMessage(const GameMessage_Player_Ready& msg) override;
        void OnGameMessage(const GameMessage_Player_Swap& msg) override;
        void OnGameMessage(const GameMessage_Map_Checksum& msg) override;
        void OnGameMessage(const GameMessage_GameCommand& msg) override;
        void OnGameMessage(const GameMessage_Server_Speed& msg) override;
        void OnGameMessage(const GameMessage_SendAsyncLog& msg) override;

        /// Sets the color of this player to the given color, if it is unique, or to the next free one if not
        /// Sends a notification to all players if the color was changed
        void CheckAndSetColor(unsigned playerIdx, unsigned newColor) override;

        /// Handles advancing of GFs, actions of AI and potentially the NWF
        void ExecuteGameFrame();
        void RunGF( bool isNWF );
        void ExecuteNWF(const unsigned currentTime);
        void CheckAndKickLaggingPlayer(const unsigned char playerIdx);
        unsigned char GetLaggingPlayer() const;
        JoinPlayerInfo& GetJoinPlayer(unsigned playerIdx) override;

    private:
        enum ServerState
        {
            SS_STOPPED = 0,
            SS_CREATING_LOBBY, // Creating game lobby (Call Start() next)
            SS_CONFIG,
            SS_GAME
        } status;

        FramesInfo framesinfo;
        unsigned currentGF;

        class ServerConfig
        {
            public:

                ServerConfig();
                void Clear();

                ServerType servertype;
                unsigned playercount;
                std::string gamename;
                std::string password;
                unsigned short port;
                bool ipv6;
                bool use_upnp;
        } config;

        MapInfo mapinfo;

        Socket serversocket;
        std::vector<GameServerPlayer> players;
        GlobalGameSettings ggs_;

        /// der Spielstartcountdown
        class CountDown
        {
            bool isActive;
            unsigned remainingSecs;
            unsigned lasttime;
        public:
            CountDown();
            /// Starts a countdown at curTime of timeInSec seconds
            void Start(unsigned timeInSec, unsigned curTime);
            void Stop();
            /// Updates the state and returns true on change. Stops 1s after remainingSecs reached zero
            bool Update(unsigned curTime);
            bool IsActive() const { return isActive; }
            unsigned GetRemainingSecs() const { return remainingSecs; }
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
		unsigned skiptogf;
};

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#define GAMESERVER GameServer::inst()

#endif
