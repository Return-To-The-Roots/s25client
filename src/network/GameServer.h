// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
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

#include "FramesInfo.h"
#include "GameMessageInterface.h"
#include "GameProtocol.h"
#include "GlobalGameSettings.h"
#include "JoinPlayerInfo.h"
#include "NWFInfo.h"
#include "helpers/Deleter.h"
#include "random/Random.h"
#include "gameTypes/MapInfo.h"
#include "gameTypes/ServerType.h"
#include "liblobby/LobbyInterface.h"
#include "libutil/LANDiscoveryService.h"
#include "libutil/Singleton.h"
#include <boost/chrono.hpp>
#include <boost/interprocess/smart_ptr/unique_ptr.hpp>
#include <vector>

class AIPlayer;
struct CreateServerInfo;
class GameMessage;
class GameMessageWithPlayer;
class GameMessage_GameCommand;
class GameServerPlayer;
struct AIServerPlayer;

class GameServer : public Singleton<GameServer, SingletonPolicies::WithLongevity>, public GameMessageInterface, public LobbyInterface
{
public:
    BOOST_STATIC_CONSTEXPR unsigned Longevity = 6;
    typedef boost::chrono::steady_clock SteadyClock;

    GameServer();
    ~GameServer() override;

    /// Starts the server
    bool Start(const CreateServerInfo& csi, const std::string& map_path, MapType map_type, const std::string& hostPw);

    void Run();

    void RunStateGame();

    void RunStateConfig();

    void Stop();

private:
    bool StartGame();

    unsigned CalcNWFLenght(FramesInfo::milliseconds32_t minDuration);

    GameServerPlayer* GetNetworkPlayer(unsigned playerId);
    /// Swap players ingame or during config
    void SwapPlayer(const uint8_t player1, const uint8_t player2);

    void SendToAll(const GameMessage& msg);
    void SendNWFDone(const NWFServerInfo& info);

    /// Kick a player (free slot and set socket to invalid. Does NOT remove it from NetworkPlayers)
    void KickPlayer(uint8_t playerId, KickReason cause, uint32_t param);

    void ClientWatchDog();

    void WaitForClients();
    void FillPlayerQueues();

    unsigned GetNumFilledSlots() const;
    /// Notifies listeners (e.g. Lobby) that the game status has changed (e.g player count)
    void AnnounceStatusChange();
    void SetPaused(bool paused);

    void LC_Status_Error(const std::string& error) override;
    void LC_Created() override;

    bool OnGameMessage(const GameMessage_Pong& msg) override;
    bool OnGameMessage(const GameMessage_Server_Type& msg) override;
    bool OnGameMessage(const GameMessage_Server_Password& msg) override;
    bool OnGameMessage(const GameMessage_Chat& msg) override;
    bool OnGameMessage(const GameMessage_GGSChange& msg) override;
    bool OnGameMessage(const GameMessage_Player_State& msg) override;
    bool OnGameMessage(const GameMessage_Player_Name& msg) override;
    bool OnGameMessage(const GameMessage_Player_Nation& msg) override;
    bool OnGameMessage(const GameMessage_Player_Team& msg) override;
    bool OnGameMessage(const GameMessage_Player_Color& msg) override;
    bool OnGameMessage(const GameMessage_Player_Ready& msg) override;
    bool OnGameMessage(const GameMessage_Player_Swap& msg) override;
    bool OnGameMessage(const GameMessage_Player_SwapConfirm& msg) override;
    bool OnGameMessage(const GameMessage_MapRequest& msg) override;
    bool OnGameMessage(const GameMessage_Map_Checksum& msg) override;
    bool OnGameMessage(const GameMessage_GameCommand& msg) override;
    bool OnGameMessage(const GameMessage_Speed& msg) override;
    bool OnGameMessage(const GameMessage_AsyncLog& msg) override;
    bool OnGameMessage(const GameMessage_RemoveLua& msg) override;
    bool OnGameMessage(const GameMessage_Countdown& msg) override;
    bool OnGameMessage(const GameMessage_CancelCountdown& msg) override;
    bool OnGameMessage(const GameMessage_Pause& msg) override;
    bool OnGameMessage(const GameMessage_SkipToGF& msg) override;
    void CancelCountdown();
    bool ArePlayersReady() const;
    /// Some player data has changed. Set non-ready and cancel countdown
    void PlayerDataChanged(unsigned playerIdx);

    /// Sets the color of this player to the given color, if it is unique, or to the next free one if not
    /// Sends a notification to all players if the color was changed
    void CheckAndSetColor(unsigned playerIdx, unsigned newColor);

    /// Handles advancing of GFs, actions of AI and potentially the NWF
    void ExecuteGameFrame();
    void ExecuteNWF();

    bool CheckForAsync();
    std::string SaveAsyncLog();
    void SendAsyncLog(const std::string& asyncLogFilePath);

    void CheckAndKickLaggingPlayers();
    bool CheckForLaggingPlayers();
    JoinPlayerInfo& GetJoinPlayer(unsigned playerIdx);

    /// Is the player with the given idx the host?
    bool IsHost(unsigned playerIdx) const;
    /// Get the player this message concerns. which is msg.player, msg.senderPlayer or -1 on error/wrong values
    int GetTargetPlayer(const GameMessageWithPlayer& msg);

    unsigned skiptogf;

    enum ServerState
    {
        SS_STOPPED = 0,
        SS_CONFIG,
        SS_LOADING,
        SS_GAME
    } state;

    FramesInfo framesinfo;
    unsigned currentGF;

    struct ServerConfig
    {
        ServerConfig();
        void Clear();

        ServerType servertype;
        std::string gamename;
        std::string hostPassword, password;
        unsigned short port;
        bool ipv6;
    } config;

    MapInfo mapinfo;

    Socket serversocket;
    std::vector<JoinPlayerInfo> playerInfos;
    std::vector<GameServerPlayer> networkPlayers;
    NWFInfo nwfInfo;
    GlobalGameSettings ggs_;

    /// der Spielstartcountdown
    class CountDown
    {
        bool isActive;
        unsigned remainingSecs;
        boost::chrono::steady_clock::time_point lasttime;

    public:
        CountDown();
        /// Starts a countdown at curTime of timeInSec seconds
        void Start(unsigned timeInSec);
        void Stop();
        /// Updates the state and returns true on change. Stops 1s after remainingSecs reached zero
        bool Update();
        bool IsActive() const { return isActive; }
        unsigned GetRemainingSecs() const { return remainingSecs; }
    } countdown;

    struct AsyncLog;
    /// AsyncLogs of all players
    std::vector<AsyncLog> asyncLogs;
    /// Time at which the loading started
    boost::chrono::steady_clock::time_point loadStartTime;

    LANDiscoveryService lanAnnouncer;
    void RunStateLoading();
};

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#define GAMESERVER GameServer::inst()

#endif
