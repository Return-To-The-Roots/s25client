// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "FramesInfo.h"
#include "GameMessageInterface.h"
#include "GameProtocol.h"
#include "GlobalGameSettings.h"
#include "JoinPlayerInfo.h"
#include "NWFInfo.h"
#include "gameTypes/MapDescription.h"
#include "gameTypes/MapInfo.h"
#include "gameTypes/ServerType.h"
#include "liblobby/LobbyInterface.h"
#include "s25util/LANDiscoveryService.h"
#include "s25util/Singleton.h"
#include <chrono>
#include <vector>

struct CreateServerInfo;
class GameMessage;
class GameMessageWithPlayer;
class GameMessage_GameCommand;
class GameServerPlayer;
struct AIServerPlayer;

class GameServer :
    public Singleton<GameServer, SingletonPolicies::WithLongevity>,
    public GameMessageInterface,
    public LobbyInterface
{
public:
    static constexpr unsigned Longevity = 6;
    using SteadyClock = std::chrono::steady_clock;

    GameServer();
    ~GameServer();

    /// Starts the server
    bool start(const CreateServerInfo& csi, const MapDescription& map, const std::string& hostPw);

    void run();

    void runStateGame();

    void runStateConfig();

    void stop();

    /// Assign players that do not have a fixed team, return true if any player was assigned.
    static bool assignPlayersOfRandomTeams(std::vector<JoinPlayerInfo>& playerInfos);

private:
    bool startGame();

    unsigned calcNWFLength(std::chrono::milliseconds minDuration) const;

    GameServerPlayer* getNetworkPlayer(unsigned playerId);
    /// Swap players ingame or during config
    void swapPlayer(uint8_t player1, uint8_t player2);

    void sendToAll(const GameMessage& msg);
    void sendNWFDone(const NWFServerInfo& info);

    /// Kick a player (free slot and set socket to invalid. Does NOT remove it from NetworkPlayers)
    void kickPlayer(uint8_t playerId, KickReason cause, uint32_t param);

    void clientWatchDog();

    void waitForClients();
    void fillPlayerQueues();

    unsigned getNumFilledSlots() const;
    /// Notifies listeners (e.g. Lobby) that the game status has changed (e.g player count)
    void announceStatusChange();
    void setPaused(bool paused);

    void LC_Status_Error(const std::string& error) override;
    void LC_Created() override;

    RTTR_IGNORE_OVERLOADED_VIRTUAL
    bool onGameMessage(const GameMessage_Pong& msg) override;
    bool onGameMessage(const GameMessage_Server_Type& msg) override;
    bool onGameMessage(const GameMessage_Server_Password& msg) override;
    bool onGameMessage(const GameMessage_Chat& msg) override;
    bool onGameMessage(const GameMessage_GGSChange& msg) override;
    bool onGameMessage(const GameMessage_Player_State& msg) override;
    bool onGameMessage(const GameMessage_Player_Name& msg) override;
    bool onGameMessage(const GameMessage_Player_Portrait& msg) override;
    bool onGameMessage(const GameMessage_Player_Nation& msg) override;
    bool onGameMessage(const GameMessage_Player_Team& msg) override;
    bool onGameMessage(const GameMessage_Player_Color& msg) override;
    bool onGameMessage(const GameMessage_Player_Ready& msg) override;
    bool onGameMessage(const GameMessage_Player_Swap& msg) override;
    bool onGameMessage(const GameMessage_Player_SwapConfirm& msg) override;
    bool onGameMessage(const GameMessage_MapRequest& msg) override;
    bool onGameMessage(const GameMessage_Map_Checksum& msg) override;
    bool onGameMessage(const GameMessage_GameCommand& msg) override;
    bool onGameMessage(const GameMessage_Speed& msg) override;
    bool onGameMessage(const GameMessage_AsyncLog& msg) override;
    bool onGameMessage(const GameMessage_RemoveLua& msg) override;
    bool onGameMessage(const GameMessage_Countdown& msg) override;
    bool onGameMessage(const GameMessage_CancelCountdown& msg) override;
    bool onGameMessage(const GameMessage_Pause& msg) override;
    bool onGameMessage(const GameMessage_SkipToGF& msg) override;
    RTTR_POP_DIAGNOSTIC

    void cancelCountdown();
    bool arePlayersReady() const;
    /// Some player data has changed. Set non-ready and cancel countdown
    void playerDataChanged(unsigned playerIdx);

    /// Sets the color of this player to the given color.
    /// If ensureUnique is true and the color is already used it will be set to the next free one
    /// Sends a notification to all players if the color was changed
    void checkAndSetColor(unsigned playerIdx, unsigned newColor, bool ensureUnique);

    /// Handles advancing of GFs, actions of AI and potentially the NWF
    void executeGameFrame();
    void executeNWF();

    bool checkForAsync();
    boost::filesystem::path saveAsyncLog();
    void sendAsyncLog(const boost::filesystem::path& asyncLogFilePath);

    void checkAndKickLaggingPlayers();
    bool checkForLaggingPlayers();
    JoinPlayerInfo& getJoinPlayer(unsigned playerIdx);

    /// Is the player with the given idx the host?
    bool isHost(unsigned playerIdx) const;
    /// Get the player this message concerns. which is msg.player, msg.senderPlayer or -1 on error/wrong values
    int getTargetPlayer(const GameMessageWithPlayer& msg);

    unsigned skiptogf;

    enum class ServerState
    {
        Stopped,
        Config,
        Loading,
        Game
    } state;

    FramesInfo framesinfo;
    unsigned currentGF;

    struct ServerConfig
    {
        ServerConfig();
        void clear();

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
        std::chrono::steady_clock::time_point lasttime;

    public:
        CountDown();
        /// Starts a countdown at curTime of timeInSec seconds
        void start(unsigned timeInSec);
        void stop();
        /// Updates the state and returns true on change. Stops 1s after remainingSecs reached zero
        bool update();
        bool IsActive() const { return isActive; }
        unsigned getRemainingSecs() const { return remainingSecs; }
    } countdown;

    struct AsyncLog;
    /// AsyncLogs of all players
    std::vector<AsyncLog> asyncLogs;
    /// Time at which the loading started
    std::chrono::steady_clock::time_point loadStartTime;

    LANDiscoveryService lanAnnouncer;
    void runStateLoading();
};

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#define GAMESERVER GameServer::inst()
