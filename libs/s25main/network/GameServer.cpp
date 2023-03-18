// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "GameServer.h"
#include "Debug.h"
#include "GameMessage.h"
#include "GameMessage_GameCommand.h"
#include "GameServerPlayer.h"
#include "GlobalGameSettings.h"
#include "JoinPlayerInfo.h"
#include "RTTR_Version.h"
#include "RttrConfig.h"
#include "Savegame.h"
#include "Settings.h"
#include "commonDefines.h"
#include "files.h"
#include "helpers/containerUtils.h"
#include "helpers/random.h"
#include "network/CreateServerInfo.h"
#include "network/GameMessages.h"
#include "random/randomIO.h"
#include "gameTypes/LanGameInfo.h"
#include "gameTypes/TeamTypes.h"
#include "gameData/GameConsts.h"
#include "gameData/LanDiscoveryCfg.h"
#include "gameData/MaxPlayers.h"
#include "liblobby/LobbyClient.h"
#include "libsiedler2/ArchivItem_Map.h"
#include "libsiedler2/ArchivItem_Map_Header.h"
#include "libsiedler2/prototypen.h"
#include "s25util/SocketSet.h"
#include "s25util/colors.h"
#include "s25util/utf8.h"
#include <boost/container/static_vector.hpp>
#include <boost/filesystem.hpp>
#include <boost/nowide/convert.hpp>
#include <boost/nowide/fstream.hpp>
#include <cmath>
#include <helpers/chronoIO.h>
#include <iomanip>
#include <iterator>
#include <mygettext/mygettext.h>

struct GameServer::AsyncLog
{
    uint8_t playerId;
    bool done;
    AsyncChecksum checksum;
    std::string addData;
    std::vector<RandomEntry> randEntries;
    AsyncLog(uint8_t playerId, AsyncChecksum checksum) : playerId(playerId), done(false), checksum(checksum) {}
};

GameServer::ServerConfig::ServerConfig()
{
    Clear();
}

void GameServer::ServerConfig::Clear()
{
    servertype = ServerType::Local;
    gamename.clear();
    password.clear();
    port = 0;
    ipv6 = false;
}

GameServer::CountDown::CountDown() : isActive(false), remainingSecs(0) {}

void GameServer::CountDown::Start(unsigned timeInSec)
{
    isActive = true;
    remainingSecs = timeInSec;
    lasttime = SteadyClock::now();
}

void GameServer::CountDown::Stop()
{
    isActive = false;
}

bool GameServer::CountDown::Update()
{
    RTTR_Assert(isActive);
    SteadyClock::time_point curTime = SteadyClock::now();

    // Check if 1s has passed
    if(curTime - lasttime < std::chrono::seconds(1))
        return false;
    if(remainingSecs == 0)
    {
        Stop();
        return true;
    }
    // 1s has passed -> Reduce remaining time
    lasttime = curTime;
    remainingSecs--;
    return true;
}

///////////////////////////////////////////////////////////////////////////////
//
GameServer::GameServer() : skiptogf(0), state(ServerState::Stopped), currentGF(0), lanAnnouncer(LAN_DISCOVERY_CFG) {}

///////////////////////////////////////////////////////////////////////////////
//
GameServer::~GameServer()
{
    Stop();
}

///////////////////////////////////////////////////////////////////////////////
// Spiel hosten
bool GameServer::Start(const CreateServerInfo& csi, const boost::filesystem::path& map_path, MapType map_type,
                       const std::string& hostPw)
{
    Stop();

    // Name, Password und Kartenname kopieren
    config.gamename = csi.gameName;
    config.hostPassword = hostPw;
    config.password = csi.password;
    config.servertype = csi.type;
    config.port = csi.port;
    config.ipv6 = csi.ipv6;
    mapinfo.type = map_type;
    mapinfo.filepath = map_path;

    // Maps, Random-Maps, Savegames - Header laden und relevante Informationen rausschreiben (Map-Titel, Spieleranzahl)
    switch(mapinfo.type)
    {
        // Altes S2-Mapformat von BB
        case MapType::OldMap:
        {
            libsiedler2::Archiv map;

            // Karteninformationen laden
            if(libsiedler2::loader::LoadMAP(mapinfo.filepath, map, true) != 0)
            {
                LOG.write("GameServer::Start: ERROR: Map %1%, couldn't load header!\n") % mapinfo.filepath;
                return false;
            }
            const libsiedler2::ArchivItem_Map_Header& header =
              checkedCast<const libsiedler2::ArchivItem_Map*>(map.get(0))->getHeader();

            playerInfos.resize(header.getNumPlayers());
            mapinfo.title = s25util::ansiToUTF8(header.getName());
            ggs_.LoadSettings();
            currentGF = 0;
        }
        break;
        case MapType::AIBattle:
        {
            mapinfo.type = MapType::OldMap;
            libsiedler2::Archiv map;

            // Karteninformationen laden
            if(libsiedler2::loader::LoadMAP(mapinfo.filepath, map, true) != 0)
            {
                LOG.write("GameServer::Start: ERROR: Map %1%, couldn't load header!\n") % mapinfo.filepath;
                return false;
            }
            const libsiedler2::ArchivItem_Map_Header& header =
              checkedCast<const libsiedler2::ArchivItem_Map*>(map.get(0))->getHeader();

            playerInfos.resize(header.getNumPlayers());
            mapinfo.title = s25util::ansiToUTF8(header.getName());
            ggs_.LoadSettings();
            currentGF = 0;
        }
        break;
        // Gespeichertes Spiel
        case MapType::Savegame:
        {
            Savegame save;

            if(!save.Load(mapinfo.filepath, SaveGameDataToLoad::HeaderAndSettings))
                return false;

            // Spieleranzahl
            playerInfos.resize(save.GetNumPlayers());
            mapinfo.title = save.GetMapName();

            for(unsigned i = 0; i < playerInfos.size(); ++i)
            {
                playerInfos[i] = JoinPlayerInfo(save.GetPlayer(i));
                // If it was a human we make it free, so someone can join
                if(playerInfos[i].ps == PlayerState::Occupied)
                    playerInfos[i].ps = PlayerState::Free;
            }

            ggs_ = save.ggs;
            currentGF = save.start_gf;
        }
        break;
    }

    if(playerInfos.empty())
    {
        LOG.write("Map %1% has no players!\n") % mapinfo.filepath;
        return false;
    }

    if(!mapinfo.mapData.CompressFromFile(mapinfo.filepath, &mapinfo.mapChecksum))
        return false;

    bfs::path luaFilePath = bfs::path(mapinfo.filepath).replace_extension("lua");
    if(bfs::is_regular_file(luaFilePath))
    {
        if(!mapinfo.luaData.CompressFromFile(luaFilePath, &mapinfo.luaChecksum))
            return false;
        mapinfo.luaFilepath = luaFilePath;
    } else
        RTTR_Assert(mapinfo.luaFilepath.empty() && mapinfo.luaChecksum == 0);

    if(!mapinfo.verifySize())
    {
        LOG.write("Map %1% is to large!\n") % mapinfo.filepath;
        return false;
    }

    // ab in die Konfiguration
    state = ServerState::Config;

    // und das socket in listen-modus schicken
    if(!serversocket.Listen(config.port, config.ipv6, csi.use_upnp))
    {
        LOG.write("GameServer::Start: ERROR: Listening on port %d failed!\n") % config.port;
        LOG.writeLastError("Fehler");
        return false;
    }

    if(config.servertype == ServerType::LAN)
        lanAnnouncer.Start();
    else if(config.servertype == ServerType::Lobby)
    {
        LOBBYCLIENT.AddServer(config.gamename, mapinfo.title, (config.password.length() != 0), config.port);
        LOBBYCLIENT.AddListener(this);
    }
    AnnounceStatusChange();

    return true;
}

unsigned GameServer::GetNumFilledSlots() const
{
    unsigned numFilled = 0;
    for(const JoinPlayerInfo& player : playerInfos)
    {
        if(player.ps != PlayerState::Free)
            ++numFilled;
    }
    return numFilled;
}

void GameServer::AnnounceStatusChange()
{
    if(config.servertype == ServerType::LAN)
    {
        LanGameInfo info;
        info.name = config.gamename;
        info.hasPwd = !config.password.empty();
        info.map = mapinfo.title;
        info.curNumPlayers = GetNumFilledSlots();
        info.maxNumPlayers = playerInfos.size();
        info.port = config.port;
        info.isIPv6 = config.ipv6;
        info.version = rttr::version::GetReadableVersion();
        info.revision = rttr::version::GetRevision();
        Serializer ser;
        info.Serialize(ser);
        lanAnnouncer.SetPayload(ser.GetData(), ser.GetLength());
    } else if(config.servertype == ServerType::Lobby)
    {
        if(LOBBYCLIENT.IsIngame())
            LOBBYCLIENT.UpdateServerNumPlayers(GetNumFilledSlots(), playerInfos.size());
    }
}

void GameServer::LC_Status_Error(const std::string& /*error*/)
{
    // Error during adding of server to lobby -> Stop
    Stop();
}

void GameServer::LC_Created()
{
    // All good -> Don't listen anymore
    LOBBYCLIENT.RemoveListener(this);
    AnnounceStatusChange();
}

///////////////////////////////////////////////////////////////////////////////
// Hauptschleife
void GameServer::Run()
{
    if(state == ServerState::Stopped)
        return;

    // auf tote Clients prüfen
    ClientWatchDog();

    // auf neue Clients warten
    if(state == ServerState::Config)
        RunStateConfig();
    else if(state == ServerState::Loading)
        RunStateLoading();
    else if(state == ServerState::Game)
        RunStateGame();

    // post zustellen
    FillPlayerQueues();

    // Execute messages
    for(GameServerPlayer& player : networkPlayers)
    {
        // Ignore kicked players
        if(!player.socket.isValid())
            continue;
        player.executeMsgs(*this);
    }
    // Send afterwards as most messages are relayed which should be done as fast as possible
    for(GameServerPlayer& player : networkPlayers)
    {
        // Ignore kicked players
        if(!player.socket.isValid())
            continue;
        player.sendMsgs(10);
    }
    helpers::erase_if(networkPlayers, [](const auto& player) { return !player.socket.isValid(); });

    lanAnnouncer.Run();
}

void GameServer::RunStateConfig()
{
    WaitForClients();
    if(countdown.IsActive() && countdown.Update())
    {
        // nun echt starten
        if(!countdown.IsActive())
        {
            if(!StartGame())
            {
                Stop();
                return;
            }
        } else
            SendToAll(GameMessage_Countdown(countdown.GetRemainingSecs()));
    }
}

void GameServer::RunStateLoading()
{
    if(!nwfInfo.isReady())
    {
        if(SteadyClock::now() - loadStartTime > std::chrono::seconds(LOAD_TIMEOUT))
        {
            for(const NWFPlayerInfo& player : nwfInfo.getPlayerInfos())
            {
                if(player.isLagging)
                    KickPlayer(player.id, KickReason::PingTimeout, __LINE__);
            }
        }
        return;
    }
    LOG.write("SERVER: Game loaded by all players after %1%\n")
      % helpers::withUnit(std::chrono::duration_cast<std::chrono::seconds>(SteadyClock::now() - loadStartTime));
    // The first NWF is ready. Server has to set up "missing" commands so every future command is for the correct NWF as
    // specified with cmdDelay. We have commands for NWF 0. When clients execute this they will send the commands for
    // NWF cmdDelay. So commands for NWF 1..cmdDelay-1 are missing. Do this here and before the NWFDone is sent,
    // otherwise we might get them in a wrong order when messages are sent asynchronously
    for(unsigned i = 1; i < nwfInfo.getCmdDelay(); i++)
    {
        for(const NWFPlayerInfo& player : nwfInfo.getPlayerInfos())
        {
            GameMessage_GameCommand msg(player.id, nwfInfo.getPlayerCmds(player.id).checksum,
                                        std::vector<gc::GameCommandPtr>());
            SendToAll(msg);
            nwfInfo.addPlayerCmds(player.id, msg.cmds);
        }
    }

    NWFServerInfo serverInfo = nwfInfo.getServerInfo();
    // Send cmdDelay NWFDone messages
    // First send the OK for NWF 0 which is also the game ready command
    // Note: Do not store. It already is in NWFInfo
    SendToAll(GameMessage_Server_NWFDone(serverInfo.gf, serverInfo.newGFLen, serverInfo.nextNWF));
    RTTR_Assert(framesinfo.nwf_length > 0);
    // Then the remaining OKs for the commands sent above
    for(unsigned i = 1; i < nwfInfo.getCmdDelay(); i++)
    {
        serverInfo.gf = serverInfo.nextNWF;
        serverInfo.nextNWF += framesinfo.nwf_length;
        SendNWFDone(serverInfo);
    }

    // And go!
    framesinfo.lastTime = FramesInfo::UsedClock::now();
    state = ServerState::Game;
}

void GameServer::RunStateGame()
{
    if(!framesinfo.isPaused)
        ExecuteGameFrame();
}

///////////////////////////////////////////////////////////////////////////////
// stoppt den server
void GameServer::Stop()
{
    if(state == ServerState::Stopped)
        return;

    // player verabschieden
    playerInfos.clear();
    networkPlayers.clear();

    // aufräumen
    framesinfo.Clear();
    config.Clear();
    mapinfo.Clear();
    countdown.Stop();

    // laden dicht machen
    serversocket.Close();
    // clear jump target
    skiptogf = 0;

    // clear async logs
    asyncLogs.clear();

    lanAnnouncer.Stop();

    if(LOBBYCLIENT.IsLoggedIn()) // steht die Lobbyverbindung noch?
        LOBBYCLIENT.DeleteServer();
    LOBBYCLIENT.RemoveListener(this);

    // status
    state = ServerState::Stopped;
    LOG.write("server state changed to stop\n");
}

// Check if there are players that have not been assigned a team but only a random team.
// Those players are assigned a team now optionally trying to balance the number of players per team.
// Returns true iff players have been assigned.
bool GameServer::assignPlayersOfRandomTeams(std::vector<JoinPlayerInfo>& playerInfos)
{
    static_assert(NUM_TEAMS == 4, "Expected exactly 4 playable teams!");
    RTTR_Assert(playerInfos.size() <= MAX_PLAYERS);

    using boost::container::static_vector;
    using PlayerIndex = unsigned;
    using TeamIndex = unsigned;
    const auto teamIdxToTeam = [](const TeamIndex teamNum) {
        RTTR_Assert(teamNum < NUM_TEAMS);
        return Team(static_cast<TeamIndex>(Team::Team1) + teamNum);
    };

    std::array<unsigned, NUM_TEAMS> numPlayersInTeam{};
    struct AssignPlayer
    {
        PlayerIndex player;
        static_vector<TeamIndex, NUM_TEAMS> possibleTeams;
        TeamIndex chosenTeam = 0;
    };

    static_vector<AssignPlayer, MAX_PLAYERS> playersToAssign;
    auto rng = helpers::getRandomGenerator();

    bool playerWasAssigned = false;

    // Assign (fully) random teams, count players in team and sort into playersToAssign
    for(PlayerIndex player = 0; player < playerInfos.size(); ++player)
    {
        auto& playerInfo = playerInfos[player];
        if(playerInfo.team == Team::Random)
        {
            const TeamIndex randTeam = std::uniform_int_distribution<TeamIndex>{0, NUM_TEAMS - 1u}(rng);
            playerInfo.team = teamIdxToTeam(randTeam);
            playerWasAssigned = true;
        }
        switch(playerInfo.team)
        {
            case Team::Team1: ++numPlayersInTeam[0]; break;
            case Team::Team2: ++numPlayersInTeam[1]; break;
            case Team::Team3: ++numPlayersInTeam[2]; break;
            case Team::Team4: ++numPlayersInTeam[3]; break;
            case Team::Random1To2: playersToAssign.emplace_back(AssignPlayer{player, {0, 1}}); break;
            case Team::Random1To3: playersToAssign.emplace_back(AssignPlayer{player, {0, 1, 2}}); break;
            case Team::Random1To4: playersToAssign.emplace_back(AssignPlayer{player, {0, 1, 2, 3}}); break;
            case Team::Random: RTTR_Assert(false); break;
            case Team::None: break;
        }
    }

    // To make the teams as even as possible we start to assign the most constrained players first
    std::sort(playersToAssign.begin(), playersToAssign.end(), [](const AssignPlayer& lhs, const AssignPlayer& rhs) {
        return lhs.possibleTeams.size() < rhs.possibleTeams.size();
    });

    // Put each player into a random team with the currently least amount of players using the possible teams only
    for(AssignPlayer& player : playersToAssign)
    {
        // Determine the teams with the minima size for the currently possible teams and choose one randomly
        unsigned minNextTeamSize = std::numeric_limits<unsigned>::max();
        static_vector<TeamIndex, NUM_TEAMS> teamsForNextPlayer;
        for(const TeamIndex team : player.possibleTeams)
        {
            if(minNextTeamSize > numPlayersInTeam[team])
            {
                teamsForNextPlayer.clear();
                teamsForNextPlayer.push_back(team);
                minNextTeamSize = numPlayersInTeam[team];
            } else if(minNextTeamSize == numPlayersInTeam[team])
                teamsForNextPlayer.push_back(team);
        }
        player.chosenTeam = helpers::getRandomElement(rng, teamsForNextPlayer);

        ++numPlayersInTeam[player.chosenTeam];
        playerWasAssigned = true;
    }
    // Now the teams are as even as possible and the uneven team(s) is a random one within the constraints
    // To have some more randomness we swap players within their constraints
    std::shuffle(playersToAssign.begin(), playersToAssign.end(), rng);
    for(auto it = playersToAssign.begin(); it != playersToAssign.end(); ++it)
    {
        // Search for a random player with which we can swap, including ourselfes
        // Go only forward to avoid back-swapping
        static_vector<decltype(it), MAX_PLAYERS> possibleSwapTargets;
        for(auto it2 = it; it2 != playersToAssign.end(); ++it2)
        {
            if(helpers::contains(it->possibleTeams, it2->chosenTeam)
               && helpers::contains(it2->possibleTeams, it->chosenTeam))
                possibleSwapTargets.push_back(it2);
        }
        const auto itSwapTarget = helpers::getRandomElement(rng, possibleSwapTargets);
        std::swap(it->chosenTeam, itSwapTarget->chosenTeam);
        playerInfos[it->player].team = teamIdxToTeam(it->chosenTeam);
    }

    return playerWasAssigned;
}

/**
 *  startet das Spiel.
 */
bool GameServer::StartGame()
{
    lanAnnouncer.Stop();

    // Finalize the team selection for unassigned players.
    if(assignPlayersOfRandomTeams(playerInfos))
        SendToAll(GameMessage_Player_List(playerInfos));

    // Bei Savegames wird der Startwert von den Clients aus der Datei gelesen!
    unsigned random_init;
    if(mapinfo.type == MapType::Savegame)
        random_init = 0;
    else
        random_init = static_cast<unsigned>(std::chrono::high_resolution_clock::now().time_since_epoch().count());

    nwfInfo.init(currentGF, 3);

    // Send start first, then load the rest
    SendToAll(GameMessage_Server_Start(random_init, nwfInfo.getNextNWF(), nwfInfo.getCmdDelay()));
    LOG.writeToFile("SERVER >>> BROADCAST: NMS_SERVER_START(%d)\n") % random_init;

    // Höchsten Ping ermitteln
    unsigned highest_ping = 0;
    for(const JoinPlayerInfo& player : playerInfos)
    {
        if(player.ps == PlayerState::Occupied)
        {
            if(player.ping > highest_ping)
                highest_ping = player.ping;
        }
    }

    framesinfo.gfLengthReq = framesinfo.gf_length = SPEED_GF_LENGTHS[ggs_.speed];

    // NetworkFrame-Länge bestimmen, je schlechter (also höher) die Pings, desto länger auch die Framelänge
    framesinfo.nwf_length = CalcNWFLenght(FramesInfo::milliseconds32_t(highest_ping));

    LOG.write("SERVER: Using gameframe length of %1%\n") % helpers::withUnit(framesinfo.gf_length);
    LOG.write("SERVER: Using networkframe length of %1% GFs (%2%)\n") % framesinfo.nwf_length
      % helpers::withUnit(framesinfo.nwf_length * framesinfo.gf_length);

    for(unsigned id = 0; id < playerInfos.size(); id++)
    {
        if(playerInfos[id].isUsed())
            nwfInfo.addPlayer(id);
    }

    // Add server info so nwfInfo can be ready but do NOT send it yet, as we wait for the player commands before sending
    // the done msg
    nwfInfo.addServerInfo(NWFServerInfo(currentGF, framesinfo.gf_length / FramesInfo::milliseconds32_t(1),
                                        currentGF + framesinfo.nwf_length));

    state = ServerState::Loading;
    loadStartTime = SteadyClock::now();

    return true;
}

unsigned GameServer::CalcNWFLenght(FramesInfo::milliseconds32_t minDuration) const
{
    constexpr unsigned maxNumGF = 20;
    for(unsigned i = 1; i < maxNumGF; ++i)
    {
        if(i * framesinfo.gf_length >= minDuration)
            return i;
    }
    return maxNumGF;
}

void GameServer::SendNWFDone(const NWFServerInfo& info)
{
    nwfInfo.addServerInfo(info);
    SendToAll(GameMessage_Server_NWFDone(info.gf, info.newGFLen, info.nextNWF));
}

/**
 *  Nachricht an Alle
 */
void GameServer::SendToAll(const GameMessage& msg)
{
    for(GameServerPlayer& player : networkPlayers)
    {
        // ist der Slot Belegt, dann Nachricht senden
        if(player.isActive())
            player.sendMsgAsync(msg.clone());
    }
}

void GameServer::KickPlayer(uint8_t playerId, KickReason cause, uint32_t param)
{
    if(playerId >= playerInfos.size())
        return;
    JoinPlayerInfo& playerInfo = playerInfos[playerId];
    GameServerPlayer* player = GetNetworkPlayer(playerId);
    if(player)
        player->closeConnection();
    // Non-existing or connecting player
    if(!playerInfo.isUsed())
        return;
    playerInfo.ps = PlayerState::Free;

    SendToAll(GameMessage_Player_Kicked(playerId, cause, param));

    // If we are ingame, replace by KI
    if(state == ServerState::Game || state == ServerState::Loading)
    {
        playerInfo.ps = PlayerState::AI;
        playerInfo.aiInfo = AI::Info(AI::Type::Dummy);
    } else
        CancelCountdown();

    AnnounceStatusChange();
    LOG.writeToFile("SERVER >>> BROADCAST: NMS_PLAYERKICKED(%d,%d,%d)\n") % unsigned(playerId) % unsigned(cause)
      % unsigned(param);
}

///////////////////////////////////////////////////////////////////////////////
// testet, ob in der Verbindungswarteschlange Clients auf Verbindung warten
void GameServer::ClientWatchDog()
{
    SocketSet set;
    set.Clear();

    // sockets zum set hinzufügen
    for(GameServerPlayer& player : networkPlayers)
        set.Add(player.socket);

    // auf fehler prüfen
    if(set.Select(0, 2) > 0)
    {
        for(const GameServerPlayer& player : networkPlayers)
        {
            if(set.InSet(player.socket))
            {
                LOG.write(_("SERVER: Error on socket of player %1%, bye bye!\n")) % player.playerId;
                KickPlayer(player.playerId, KickReason::ConnectionLost, __LINE__);
            }
        }
    }

    for(GameServerPlayer& player : networkPlayers)
    {
        if(player.hasTimedOut())
        {
            LOG.write(_("SERVER: Reserved slot %1% freed due to timeout\n")) % player.playerId;
            KickPlayer(player.playerId, KickReason::PingTimeout, __LINE__);
        } else
            player.doPing();
    }
}

void GameServer::ExecuteGameFrame()
{
    RTTR_Assert(state == ServerState::Game);

    FramesInfo::UsedClock::time_point currentTime = FramesInfo::UsedClock::now();
    FramesInfo::milliseconds32_t passedTime =
      std::chrono::duration_cast<FramesInfo::milliseconds32_t>(currentTime - framesinfo.lastTime);

    // prüfen ob GF vergangen
    if(passedTime >= framesinfo.gf_length || skiptogf > currentGF)
    {
        // NWF vergangen?
        if(currentGF == nwfInfo.getNextNWF())
        {
            if(CheckForLaggingPlayers())
            {
                // Check for kicking every second
                static FramesInfo::UsedClock::time_point lastLagKickTime;
                if(currentTime - lastLagKickTime >= std::chrono::seconds(1))
                {
                    lastLagKickTime = currentTime;
                    CheckAndKickLaggingPlayers();
                }
                // Skip the rest
                return;
            } else
                ExecuteNWF();
        }
        // Advance GF
        ++currentGF;
        // Normally we set lastTime = curTime (== lastTime + passedTime) where passedTime is ideally 1 GF
        // But we might got called late, so we advance the time by 1 GF anyway so in that case we execute the next GF a
        // bit earlier. Exception: We lag many GFs behind, then we advance by the full passedTime - 1 GF which means we
        // are now only 1 GF behind and execute that on the next call
        if(passedTime <= 4 * framesinfo.gf_length)
            passedTime = framesinfo.gf_length;
        else
            passedTime -= framesinfo.gf_length;
        framesinfo.lastTime += passedTime;
    }
}

void GameServer::ExecuteNWF()
{
    // Check for asyncs
    if(CheckForAsync())
    {
        // Pause game
        RTTR_Assert(!framesinfo.isPaused);
        SetPaused(true);

        // Notify players
        std::vector<unsigned> checksumHashes;
        for(const GameServerPlayer& player : networkPlayers)
            checksumHashes.push_back(nwfInfo.getPlayerCmds(player.playerId).checksum.getHash());
        SendToAll(GameMessage_Server_Async(checksumHashes));

        // Request async logs
        for(GameServerPlayer& player : networkPlayers)
        {
            asyncLogs.push_back(AsyncLog(player.playerId, nwfInfo.getPlayerCmds(player.playerId).checksum));
            player.sendMsgAsync(new GameMessage_GetAsyncLog());
        }
    }
    const NWFServerInfo serverInfo = nwfInfo.getServerInfo();
    RTTR_Assert(serverInfo.gf == currentGF);
    RTTR_Assert(serverInfo.nextNWF > currentGF);
    // First save old values
    unsigned lastNWF = nwfInfo.getLastNWF();
    FramesInfo::milliseconds32_t oldGFLen = framesinfo.gf_length;
    nwfInfo.execute(framesinfo);
    if(oldGFLen != framesinfo.gf_length)
    {
        LOG.write(_("SERVER: At GF %1%: Speed changed from %2% to %3%. NWF %4%\n")) % currentGF
          % helpers::withUnit(oldGFLen) % helpers::withUnit(framesinfo.gf_length) % framesinfo.nwf_length;
    }
    NWFServerInfo newInfo(lastNWF, framesinfo.gfLengthReq / FramesInfo::milliseconds32_t(1),
                          lastNWF + framesinfo.nwf_length);
    if(framesinfo.gfLengthReq != framesinfo.gf_length)
    {
        // Speed will change, adjust nwf length so the time will stay constant
        using namespace std::chrono;
        using MsDouble = duration<double, std::milli>;
        double newNWFLen =
          framesinfo.nwf_length * framesinfo.gf_length / duration_cast<MsDouble>(framesinfo.gfLengthReq);
        newInfo.nextNWF = lastNWF + std::max(1l, std::lround(newNWFLen));
    }
    SendNWFDone(newInfo);
}

bool GameServer::CheckForAsync()
{
    if(networkPlayers.empty())
        return false;
    bool isAsync = false;
    const AsyncChecksum& refChecksum = nwfInfo.getPlayerCmds(networkPlayers.front().playerId).checksum;
    for(const GameServerPlayer& player : networkPlayers)
    {
        const AsyncChecksum& curChecksum = nwfInfo.getPlayerCmds(player.playerId).checksum;

        // Checksummen nicht gleich?
        if(curChecksum != refChecksum)
        {
            LOG.write(_("Async at GF %1% of player %2% vs %3%. Checksums:\n%4%\n%5%\n\n")) % currentGF % player.playerId
              % networkPlayers.front().playerId % curChecksum % refChecksum;
            isAsync = true;
        }
    }
    return isAsync;
}

void GameServer::CheckAndKickLaggingPlayers()
{
    for(const GameServerPlayer& player : networkPlayers)
    {
        const unsigned timeOut = player.getLagTimeOut();
        if(timeOut == 0)
            KickPlayer(player.playerId, KickReason::PingTimeout, __LINE__);
        else if(timeOut <= 30
                && (timeOut % 5 == 0
                    || timeOut < 5)) // Notify every 5s if max 30s are remaining, if less than 5s notify every second
            LOG.write(_("SERVER: Kicking player %1% in %2% seconds\n")) % player.playerId % timeOut;
    }
}

bool GameServer::CheckForLaggingPlayers()
{
    if(nwfInfo.isReady())
        return false;
    for(GameServerPlayer& player : networkPlayers)
    {
        if(nwfInfo.getPlayerInfo(player.playerId).isLagging)
            player.setLagging();
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
// testet, ob in der Verbindungswarteschlange Clients auf Verbindung warten
void GameServer::WaitForClients()
{
    SocketSet set;

    set.Add(serversocket);
    if(set.Select(0, 0) > 0)
    {
        RTTR_Assert(set.InSet(serversocket));
        Socket socket = serversocket.Accept();

        // Verbindung annehmen
        if(!socket.isValid())
            return;

        unsigned newPlayerId = GameMessageWithPlayer::NO_PLAYER_ID;
        // Geeigneten Platz suchen
        for(unsigned playerId = 0; playerId < playerInfos.size(); ++playerId)
        {
            if(playerInfos[playerId].ps == PlayerState::Free && !GetNetworkPlayer(playerId))
            {
                networkPlayers.push_back(GameServerPlayer(playerId, socket));
                newPlayerId = playerId;
                break;
            }
        }

        GameMessage_Player_Id msg(newPlayerId);
        MessageHandler::send(socket, msg);

        // war kein platz mehr frei, wenn ja dann verbindung trennen?
        if(newPlayerId == 0xFFFFFFFF)
            socket.Close();
    }
}

///////////////////////////////////////////////////////////////////////////////
// füllt die warteschlangen mit "paketen"
void GameServer::FillPlayerQueues()
{
    SocketSet set;
    bool msgReceived = false;

    // erstmal auf Daten überprüfen
    do
    {
        // sockets zum set hinzufügen
        for(const GameServerPlayer& player : networkPlayers)
            set.Add(player.socket);

        msgReceived = false;

        // ist eines der Sockets im Set lesbar?
        if(set.Select(0, 0) > 0)
        {
            for(GameServerPlayer& player : networkPlayers)
            {
                if(set.InSet(player.socket))
                {
                    // nachricht empfangen
                    if(!player.receiveMsgs())
                    {
                        LOG.write(_("SERVER: Receiving Message for player %1% failed, kicking...\n")) % player.playerId;
                        KickPlayer(player.playerId, KickReason::ConnectionLost, __LINE__);
                    } else
                        msgReceived = true;
                }
            }
        }
    } while(msgReceived);
}

///////////////////////////////////////////////////////////////////////////////
// pongnachricht
bool GameServer::OnGameMessage(const GameMessage_Pong& msg)
{
    GameServerPlayer* player = GetNetworkPlayer(msg.senderPlayerID);
    if(player)
    {
        unsigned ping = player->calcPingTime();
        if(ping == 0u)
            return true;
        playerInfos[msg.senderPlayerID].ping = ping;
        SendToAll(GameMessage_Player_Ping(msg.senderPlayerID, ping));
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
// servertype
bool GameServer::OnGameMessage(const GameMessage_Server_Type& msg)
{
    if(state != ServerState::Config)
    {
        KickPlayer(msg.senderPlayerID, KickReason::InvalidMsg, __LINE__);
        return true;
    }

    GameServerPlayer* player = GetNetworkPlayer(msg.senderPlayerID);
    if(!player)
        return true;

    auto typeok = GameMessage_Server_TypeOK::StatusCode::Ok;
    if(msg.type != config.servertype)
        typeok = GameMessage_Server_TypeOK::StatusCode::InvalidServerType;
    else if(msg.revision != rttr::version::GetRevision())
        typeok = GameMessage_Server_TypeOK::StatusCode::WrongVersion;

    player->sendMsg(GameMessage_Server_TypeOK(typeok, rttr::version::GetRevision()));

    if(typeok != GameMessage_Server_TypeOK::StatusCode::Ok)
        KickPlayer(msg.senderPlayerID, KickReason::ConnectionLost, __LINE__);
    return true;
}

/**
 *  Server-Passwort-Nachricht
 */
bool GameServer::OnGameMessage(const GameMessage_Server_Password& msg)
{
    if(state != ServerState::Config)
    {
        KickPlayer(msg.senderPlayerID, KickReason::InvalidMsg, __LINE__);
        return true;
    }

    GameServerPlayer* player = GetNetworkPlayer(msg.senderPlayerID);
    if(!player)
        return true;

    std::string passwordok = (config.password == msg.password ? "true" : "false");
    if(msg.password == config.hostPassword)
    {
        passwordok = "true";
        playerInfos[msg.senderPlayerID].isHost = true;
    } else
        playerInfos[msg.senderPlayerID].isHost = false;

    player->sendMsgAsync(new GameMessage_Server_Password(passwordok));

    if(passwordok == "false")
        KickPlayer(msg.senderPlayerID, KickReason::WrongPassword, __LINE__);
    return true;
}

/**
 *  Chat-Nachricht.
 */
bool GameServer::OnGameMessage(const GameMessage_Chat& msg)
{
    int playerID = GetTargetPlayer(msg);
    if(playerID >= 0)
        SendToAll(GameMessage_Chat(playerID, msg.destination, msg.text));
    return true;
}

bool GameServer::OnGameMessage(const GameMessage_Player_State& msg)
{
    if(state != ServerState::Config)
    {
        KickPlayer(msg.senderPlayerID, KickReason::InvalidMsg, __LINE__);
        return true;
    }
    // Can't do this. Have to have a joined player
    if(msg.ps == PlayerState::Occupied)
        return true;

    int playerID = GetTargetPlayer(msg);
    if(playerID < 0)
        return true;
    JoinPlayerInfo& player = playerInfos[playerID];
    const PlayerState oldPs = player.ps;
    // Can't change self
    if(playerID != msg.senderPlayerID)
    {
        // oh ein spieler, weg mit ihm!
        if(GetNetworkPlayer(playerID))
            KickPlayer(playerID, KickReason::NoCause, __LINE__);

        if(mapinfo.type == MapType::Savegame)
        {
            // For savegames we cannot set anyone on a locked slot as the player does not exist on the map
            if(player.ps != PlayerState::Locked)
            {
                // And we don't lock!
                player.ps = msg.ps == PlayerState::Locked ? PlayerState::Free : msg.ps;
                player.aiInfo = msg.aiInfo;
            }
        } else
        {
            player.ps = msg.ps;
            player.aiInfo = msg.aiInfo;
        }
        if(player.ps == PlayerState::Free && config.servertype == ServerType::Local)
        {
            player.ps = PlayerState::AI;
            player.aiInfo = AI::Info(AI::Type::Default);
        }
    }
    // Even when nothing changed we send the data because the other players might have expected a change

    if(player.ps == PlayerState::AI)
    {
        player.SetAIName(playerID);
        SendToAll(GameMessage_Player_Name(playerID, player.name));
    }
    // If slot is filled, check current color
    if(player.isUsed())
        CheckAndSetColor(playerID, player.color);
    SendToAll(GameMessage_Player_State(playerID, player.ps, player.aiInfo));

    if(oldPs != player.ps)
        player.isReady = (player.ps == PlayerState::AI);
    SendToAll(GameMessage_Player_Ready(playerID, player.isReady));
    PlayerDataChanged(playerID);
    AnnounceStatusChange();
    return true;
}

///////////////////////////////////////////////////////////////////////////////
// Spielername
bool GameServer::OnGameMessage(const GameMessage_Player_Name& msg)
{
    if(state != ServerState::Config)
    {
        KickPlayer(msg.senderPlayerID, KickReason::InvalidMsg, __LINE__);
        return true;
    }
    int playerID = GetTargetPlayer(msg);
    if(playerID < 0)
        return true;

    LOG.writeToFile("CLIENT%d >>> SERVER: NMS_PLAYER_NAME(%s)\n") % playerID % msg.playername;

    playerInfos[playerID].name = msg.playername;
    SendToAll(GameMessage_Player_Name(playerID, msg.playername));
    PlayerDataChanged(playerID);

    return true;
}

///////////////////////////////////////////////////////////////////////////////
// Nation weiterwechseln
bool GameServer::OnGameMessage(const GameMessage_Player_Nation& msg)
{
    if(state != ServerState::Config)
    {
        KickPlayer(msg.senderPlayerID, KickReason::InvalidMsg, __LINE__);
        return true;
    }
    int playerID = GetTargetPlayer(msg);
    if(playerID < 0)
        return true;

    playerInfos[playerID].nation = msg.nation;

    SendToAll(GameMessage_Player_Nation(playerID, msg.nation));
    PlayerDataChanged(playerID);
    return true;
}

///////////////////////////////////////////////////////////////////////////////
// Team weiterwechseln
bool GameServer::OnGameMessage(const GameMessage_Player_Team& msg)
{
    if(state != ServerState::Config)
    {
        KickPlayer(msg.senderPlayerID, KickReason::InvalidMsg, __LINE__);
        return true;
    }
    int playerID = GetTargetPlayer(msg);
    if(playerID < 0)
        return true;

    playerInfos[playerID].team = msg.team;

    SendToAll(GameMessage_Player_Team(playerID, msg.team));
    PlayerDataChanged(playerID);
    return true;
}

///////////////////////////////////////////////////////////////////////////////
// Farbe weiterwechseln
bool GameServer::OnGameMessage(const GameMessage_Player_Color& msg)
{
    if(state != ServerState::Config)
    {
        KickPlayer(msg.senderPlayerID, KickReason::InvalidMsg, __LINE__);
        return true;
    }
    int playerID = GetTargetPlayer(msg);
    if(playerID < 0)
        return true;

    CheckAndSetColor(playerID, msg.color);
    PlayerDataChanged(playerID);
    return true;
}

/**
 *  Spielerstatus wechseln
 */
bool GameServer::OnGameMessage(const GameMessage_Player_Ready& msg)
{
    if(state != ServerState::Config)
    {
        KickPlayer(msg.senderPlayerID, KickReason::InvalidMsg, __LINE__);
        return true;
    }
    int playerID = GetTargetPlayer(msg);
    if(playerID < 0)
        return true;

    JoinPlayerInfo& player = playerInfos[playerID];

    player.isReady = msg.ready;

    // countdown ggf abbrechen
    if(!player.isReady && countdown.IsActive())
        CancelCountdown();

    SendToAll(GameMessage_Player_Ready(playerID, msg.ready));
    return true;
}

bool GameServer::OnGameMessage(const GameMessage_MapRequest& msg)
{
    if(state != ServerState::Config)
    {
        KickPlayer(msg.senderPlayerID, KickReason::InvalidMsg, __LINE__);
        return true;
    }
    GameServerPlayer* player = GetNetworkPlayer(msg.senderPlayerID);
    if(!player)
        return true;

    if(msg.requestInfo)
    {
        player->sendMsgAsync(new GameMessage_Map_Info(mapinfo.filepath.filename().string(), mapinfo.type,
                                                      mapinfo.mapData.uncompressedLength, mapinfo.mapData.data.size(),
                                                      mapinfo.luaData.uncompressedLength, mapinfo.luaData.data.size()));
    } else if(player->isMapSending())
    {
        // Don't send again
        KickPlayer(msg.senderPlayerID, KickReason::InvalidMsg, __LINE__);
    } else
    {
        // Send map data
        unsigned curPos = 0;
        unsigned remainingSize = mapinfo.mapData.data.size();
        while(remainingSize)
        {
            unsigned chunkSize = std::min(MAP_PART_SIZE, remainingSize);

            player->sendMsgAsync(new GameMessage_Map_Data(true, curPos, &mapinfo.mapData.data[curPos], chunkSize));
            curPos += chunkSize;
            remainingSize -= chunkSize;
        }

        // And lua data (if there is any)
        RTTR_Assert(mapinfo.luaFilepath.empty() == mapinfo.luaData.data.empty());
        RTTR_Assert(mapinfo.luaData.data.empty() == (mapinfo.luaData.uncompressedLength == 0));
        curPos = 0;
        remainingSize = mapinfo.luaData.data.size();
        while(remainingSize)
        {
            unsigned chunkSize = std::min(MAP_PART_SIZE, remainingSize);

            player->sendMsgAsync(new GameMessage_Map_Data(false, curPos, &mapinfo.luaData.data[curPos], chunkSize));
            curPos += chunkSize;
            remainingSize -= chunkSize;
        }
        // estimate time. max 60 chunks/s (currently limited by framerate), assume 50 (~25kb/s)
        auto numChunks = (mapinfo.mapData.data.size() + mapinfo.luaData.data.size()) / MAP_PART_SIZE;
        player->setMapSending(std::chrono::seconds(numChunks / 50 + 1));
    }
    return true;
}

bool GameServer::OnGameMessage(const GameMessage_Map_Checksum& msg)
{
    if(state != ServerState::Config)
    {
        KickPlayer(msg.senderPlayerID, KickReason::InvalidMsg, __LINE__);
        return true;
    }
    GameServerPlayer* player = GetNetworkPlayer(msg.senderPlayerID);
    if(!player)
        return true;

    bool checksumok = (msg.mapChecksum == mapinfo.mapChecksum && msg.luaChecksum == mapinfo.luaChecksum);

    LOG.writeToFile("CLIENT%d >>> SERVER: NMS_MAP_CHECKSUM(%u) expected: %u, ok: %s\n") % unsigned(msg.senderPlayerID)
      % msg.mapChecksum % mapinfo.mapChecksum % (checksumok ? "yes" : "no");

    // Send response. If map data was not sent yet, the client may retry
    player->sendMsgAsync(new GameMessage_Map_ChecksumOK(checksumok, !player->isMapSending()));

    LOG.writeToFile("SERVER >>> CLIENT%d: NMS_MAP_CHECKSUM(%d)\n") % unsigned(msg.senderPlayerID) % checksumok;

    if(!checksumok)
    {
        if(player->isMapSending())
            KickPlayer(msg.senderPlayerID, KickReason::WrongChecksum, __LINE__);
    } else
    {
        JoinPlayerInfo& playerInfo = playerInfos[msg.senderPlayerID];
        // Used? Then we got this twice or some error happened. Remove him
        if(playerInfo.isUsed())
            KickPlayer(msg.senderPlayerID, KickReason::InvalidMsg, __LINE__);
        else
        {
            // Inform others about a new player
            SendToAll(GameMessage_Player_New(msg.senderPlayerID, playerInfo.name));

            LOG.writeToFile("SERVER >>> BROADCAST: NMS_PLAYER_NEW(%d, %s)\n") % unsigned(msg.senderPlayerID)
              % playerInfo.name;

            // Mark as used and assign a unique color
            // Do this before sending the player list to avoid sending useless updates
            playerInfo.ps = PlayerState::Occupied;
            CheckAndSetColor(msg.senderPlayerID, playerInfo.color);

            // Send remaining data and mark as active
            player->sendMsgAsync(new GameMessage_Server_Name(config.gamename));
            player->sendMsgAsync(new GameMessage_Player_List(playerInfos));
            player->sendMsgAsync(new GameMessage_GGSChange(ggs_));
            player->setActive();
        }
        AnnounceStatusChange();
    }
    return true;
}

// speed change message
bool GameServer::OnGameMessage(const GameMessage_Speed& msg)
{
    if(state != ServerState::Game)
    {
        KickPlayer(msg.senderPlayerID, KickReason::InvalidMsg, __LINE__);
        return true;
    }
    framesinfo.gfLengthReq = FramesInfo::milliseconds32_t(msg.gf_length);
    return true;
}

bool GameServer::OnGameMessage(const GameMessage_GameCommand& msg)
{
    int targetPlayerId = GetTargetPlayer(msg);
    if((state != ServerState::Game && state != ServerState::Loading) || targetPlayerId < 0
       || (state == ServerState::Loading && !msg.cmds.gcs.empty()))
    {
        KickPlayer(msg.senderPlayerID, KickReason::InvalidMsg, __LINE__);
        return true;
    }

    if(!nwfInfo.addPlayerCmds(targetPlayerId, msg.cmds))
        return true; // Ignore
    GameServerPlayer* player = GetNetworkPlayer(targetPlayerId);
    if(player)
        player->setNotLagging();
    SendToAll(GameMessage_GameCommand(targetPlayerId, msg.cmds.checksum, msg.cmds.gcs));

    return true;
}

bool GameServer::OnGameMessage(const GameMessage_AsyncLog& msg)
{
    if(state != ServerState::Game)
    {
        KickPlayer(msg.senderPlayerID, KickReason::InvalidMsg, __LINE__);
        return true;
    }
    bool foundPlayer = false;
    for(AsyncLog& log : asyncLogs)
    {
        if(log.playerId != msg.senderPlayerID)
            continue;
        RTTR_Assert(!log.done);
        if(log.done)
            return true;
        foundPlayer = true;
        log.addData += msg.addData;
        log.randEntries.insert(log.randEntries.end(), msg.entries.begin(), msg.entries.end());
        if(msg.last)
        {
            LOG.write(_("Received async logs from %1% (%2% entries).\n")) % unsigned(log.playerId)
              % log.randEntries.size();
            log.done = true;
        }
    }
    if(!foundPlayer)
    {
        LOG.write(_("Received async log from %1%, but did not expect it!\n")) % unsigned(msg.senderPlayerID);
        return true;
    }

    // Check if we have all logs
    for(const AsyncLog& log : asyncLogs)
    {
        if(!log.done)
            return true;
    }

    LOG.write(_("Async logs received completely.\n"));

    const bfs::path asyncFilePath = SaveAsyncLog();
    if(!asyncFilePath.empty())
        SendAsyncLog(asyncFilePath);

    // Kick all players that have a different checksum from the host
    AsyncChecksum hostChecksum;
    for(const AsyncLog& log : asyncLogs)
    {
        if(playerInfos.at(log.playerId).isHost)
        {
            hostChecksum = log.checksum;
            break;
        }
    }
    for(const AsyncLog& log : asyncLogs)
    {
        if(log.checksum != hostChecksum)
            KickPlayer(log.playerId, KickReason::Async, __LINE__);
    }
    return true;
}

bool GameServer::OnGameMessage(const GameMessage_RemoveLua& msg)
{
    if(state != ServerState::Config || !IsHost(msg.senderPlayerID))
    {
        KickPlayer(msg.senderPlayerID, KickReason::InvalidMsg, __LINE__);
        return true;
    }
    mapinfo.luaFilepath.clear();
    mapinfo.luaData.Clear();
    mapinfo.luaChecksum = 0;
    SendToAll(msg);
    CancelCountdown();
    return true;
}

bool GameServer::OnGameMessage(const GameMessage_Countdown& msg)
{
    if(state != ServerState::Config || !IsHost(msg.senderPlayerID))
    {
        KickPlayer(msg.senderPlayerID, KickReason::InvalidMsg, __LINE__);
        return true;
    }

    NetworkPlayer* nwPlayer = GetNetworkPlayer(msg.senderPlayerID);
    if(!nwPlayer || countdown.IsActive())
        return true;

    if(!ArePlayersReady())
        nwPlayer->sendMsgAsync(new GameMessage_CancelCountdown(true));
    else
    {
        // Just to make sure update all player infos
        SendToAll(GameMessage_Player_List(playerInfos));
        // Start countdown (except its single player)
        if(networkPlayers.size() > 1)
        {
            countdown.Start(msg.countdown);
            SendToAll(GameMessage_Countdown(countdown.GetRemainingSecs()));
            LOG.writeToFile("SERVER >>> Countdown started(%d)\n") % countdown.GetRemainingSecs();
        } else if(!StartGame())
            Stop();
    }

    return true;
}

bool GameServer::OnGameMessage(const GameMessage_CancelCountdown& msg)
{
    if(state != ServerState::Config || !IsHost(msg.senderPlayerID))
    {
        KickPlayer(msg.senderPlayerID, KickReason::InvalidMsg, __LINE__);
        return true;
    }

    CancelCountdown();
    return true;
}

bool GameServer::OnGameMessage(const GameMessage_Pause& msg)
{
    if(IsHost(msg.senderPlayerID))
        SetPaused(msg.paused);
    return true;
}

bool GameServer::OnGameMessage(const GameMessage_SkipToGF& msg)
{
    if(IsHost(msg.senderPlayerID))
    {
        skiptogf = msg.targetGF;
        SendToAll(msg);
    }
    return true;
}

bool GameServer::OnGameMessage(const GameMessage_GGSChange& msg)
{
    if(state != ServerState::Config || !IsHost(msg.senderPlayerID))
    {
        KickPlayer(msg.senderPlayerID, KickReason::InvalidMsg, __LINE__);
        return true;
    }
    ggs_ = msg.ggs;
    SendToAll(msg);
    CancelCountdown();
    return true;
}

void GameServer::CancelCountdown()
{
    if(!countdown.IsActive())
        return;
    // Countdown-Stop allen mitteilen
    countdown.Stop();
    SendToAll(GameMessage_CancelCountdown());
    LOG.writeToFile("SERVER >>> BROADCAST: NMS_CANCELCOUNTDOWN\n");
}

bool GameServer::ArePlayersReady() const
{
    // Alle Spieler da?
    for(const JoinPlayerInfo& player : playerInfos)
    {
        // noch nicht alle spieler da -> feierabend!
        if(player.ps == PlayerState::Free || (player.isHuman() && !player.isReady))
            return false;
    }

    std::set<unsigned> takenColors;

    // Check all players have different colors
    for(const JoinPlayerInfo& player : playerInfos)
    {
        if(player.isUsed())
        {
            if(helpers::contains(takenColors, player.color))
                return false;
            takenColors.insert(player.color);
        }
    }
    return true;
}

void GameServer::PlayerDataChanged(unsigned playerIdx)
{
    CancelCountdown();
    JoinPlayerInfo& player = GetJoinPlayer(playerIdx);
    if(player.ps != PlayerState::AI && player.isReady)
    {
        player.isReady = false;
        SendToAll(GameMessage_Player_Ready(playerIdx, false));
    }
}

bfs::path GameServer::SaveAsyncLog()
{
    // Get the highest common counter number and start from there (remove all others)
    unsigned maxCtr = 0;
    for(const AsyncLog& log : asyncLogs)
    {
        if(!log.randEntries.empty() && log.randEntries[0].counter > maxCtr)
            maxCtr = log.randEntries[0].counter;
    }
    // Number of entries = max(asyncLogs[0..n].randEntries.size())
    unsigned numEntries = 0;
    for(AsyncLog& log : asyncLogs)
    {
        auto it = std::find_if(log.randEntries.begin(), log.randEntries.end(),
                               [maxCtr](const auto& e) { return e.counter == maxCtr; });
        log.randEntries.erase(log.randEntries.begin(), it);
        if(numEntries < log.randEntries.size())
            numEntries = log.randEntries.size();
    }
    // No entries :(
    if(numEntries == 0 || asyncLogs.size() < 2u)
        return "";

    // count identical lines
    unsigned numIdentical = 0;
    for(unsigned i = 0; i < numEntries; i++)
    {
        bool isIdentical = true;
        if(i >= asyncLogs[0].randEntries.size())
            break;
        const RandomEntry& refEntry = asyncLogs[0].randEntries[i];
        for(const AsyncLog& log : asyncLogs)
        {
            if(i >= log.randEntries.size())
            {
                isIdentical = false;
                break;
            }
            const RandomEntry& curEntry = log.randEntries[i];
            if(curEntry.maxExcl != refEntry.maxExcl || curEntry.rngState != refEntry.rngState
               || curEntry.objId != refEntry.objId)
            {
                isIdentical = false;
                break;
            }
        }
        if(isIdentical)
            ++numIdentical;
        else
            break;
    }

    LOG.write(_("There are %1% identical async log entries.\n")) % numIdentical;

    bfs::path filePath =
      RTTRCONFIG.ExpandPath(s25::folders::logs) / (s25util::Time::FormatTime("async_%Y-%m-%d_%H-%i-%s") + "Server.log");

    // open async log
    bnw::ofstream file(filePath);

    if(file)
    {
        file << "Map: " << mapinfo.title << std::endl;
        file << std::setfill(' ');
        for(const AsyncLog& log : asyncLogs)
        {
            const JoinPlayerInfo& plInfo = playerInfos.at(log.playerId);
            file << "Player " << std::setw(2) << unsigned(log.playerId) << (plInfo.isHost ? '#' : ' ') << "\t\""
                 << plInfo.name << '"' << std::endl;
            file << "System info: " << log.addData << std::endl;
            file << "\tChecksum: " << std::setw(0) << log.checksum << std::endl;
        }
        for(const AsyncLog& log : asyncLogs)
            file << "Checksum " << std::setw(2) << unsigned(log.playerId) << std::setw(0) << ": " << log.checksum
                 << std::endl;

        // print identical lines, they help in tracing the bug
        for(unsigned i = 0; i < numIdentical; i++)
            file << "[ I ]: " << asyncLogs[0].randEntries[i] << "\n";
        for(unsigned i = numIdentical; i < numEntries; i++)
        {
            for(const AsyncLog& log : asyncLogs)
            {
                if(i < log.randEntries.size())
                    file << "[C" << std::setw(2) << unsigned(log.playerId) << std::setw(0)
                         << "]: " << log.randEntries[i] << '\n';
            }
        }

        LOG.write(_("Async log saved at %1%\n")) % filePath;
        return filePath;
    } else
    {
        LOG.write(_("Failed to save async log at %1%\n")) % filePath;
        return "";
    }
}

void GameServer::SendAsyncLog(const bfs::path& asyncLogFilePath)
{
    if(SETTINGS.global.submit_debug_data == 1
#ifdef _WIN32
       || (MessageBoxW(nullptr,
                       boost::nowide::widen(_("The game clients are out of sync. Would you like to send debug "
                                              "information to RttR to help us avoiding this in "
                                              "the future? Thank you very much!"))
                         .c_str(),
                       boost::nowide::widen(_("Error")).c_str(),
                       MB_YESNO | MB_ICONERROR | MB_TASKMODAL | MB_SETFOREGROUND)
           == IDYES)
#endif
    )
    {
        DebugInfo di;
        LOG.write(_("Sending async logs %1%.\n")) % (di.SendAsyncLog(asyncLogFilePath) ? "succeeded" : "failed");

        di.SendReplay();
    }
}

void GameServer::CheckAndSetColor(unsigned playerIdx, unsigned newColor)
{
    RTTR_Assert(playerIdx < playerInfos.size());
    RTTR_Assert(playerInfos.size() <= PLAYER_COLORS.size()); // Else we may not find a valid color!

    JoinPlayerInfo& player = playerInfos[playerIdx];
    RTTR_Assert(player.isUsed()); // Should only set colors for taken spots

    // Get colors used by other players
    std::set<unsigned> takenColors;
    for(unsigned p = 0; p < playerInfos.size(); ++p)
    {
        // Skip self
        if(p == playerIdx)
            continue;

        JoinPlayerInfo& otherPlayer = playerInfos[p];
        if(otherPlayer.isUsed())
            takenColors.insert(otherPlayer.color);
    }

    // Look for a unique color
    int newColorIdx = JoinPlayerInfo::GetColorIdx(newColor);
    while(helpers::contains(takenColors, newColor))
        newColor = PLAYER_COLORS[(++newColorIdx) % PLAYER_COLORS.size()];

    if(player.color == newColor)
        return;

    player.color = newColor;

    SendToAll(GameMessage_Player_Color(playerIdx, player.color));
    LOG.writeToFile("SERVER >>> BROADCAST: NMS_PLAYER_TOGGLECOLOR(%d, %d)\n") % playerIdx % player.color;
}

bool GameServer::OnGameMessage(const GameMessage_Player_Swap& msg)
{
    if(state != ServerState::Game && state != ServerState::Config)
        return true;
    int targetPlayer = GetTargetPlayer(msg);
    if(targetPlayer < 0)
        return true;
    auto player1 = static_cast<uint8_t>(targetPlayer);
    if(player1 == msg.player2 || msg.player2 >= playerInfos.size())
        return true;

    SwapPlayer(player1, msg.player2);
    return true;
}

bool GameServer::OnGameMessage(const GameMessage_Player_SwapConfirm& msg)
{
    GameServerPlayer* player = GetNetworkPlayer(msg.senderPlayerID);
    if(!player)
        return true;
    for(auto it = player->getPendingSwaps().begin(); it != player->getPendingSwaps().end(); ++it)
    {
        if(it->first == msg.player && it->second == msg.player2)
        {
            player->getPendingSwaps().erase(it);
            break;
        }
    }
    return true;
}

void GameServer::SwapPlayer(const uint8_t player1, const uint8_t player2)
{
    // TODO: Swapping the player messes up the ids because our IDs are indizes. Usually this works because we use the
    // sender player ID for messages received by the server and set the right ID for messages sent by the server.
    // However (currently only) the host may send messages for another player. Those will not get the adjusted ID till
    // he gets the swap message. So there is a short time, where the messages may be executed for the wrong player.
    // Idea: Use actual IDs for players in messages (unique)
    if(state == ServerState::Config)
    {
        // Swap player during match-making
        // Swap everything
        using std::swap;
        swap(playerInfos[player1], playerInfos[player2]);
        // In savegames some things cannot be changed
        if(mapinfo.type == MapType::Savegame)
            playerInfos[player1].FixSwappedSaveSlot(playerInfos[player2]);
    } else if(state == ServerState::Game)
    {
        // Ingame we can only switch to a KI
        if(playerInfos[player1].ps != PlayerState::Occupied || playerInfos[player2].ps != PlayerState::AI)
            return;

        LOG.write("GameServer::ChangePlayer %i - %i \n") % unsigned(player1) % unsigned(player2);
        using std::swap;
        swap(playerInfos[player2].ps, playerInfos[player1].ps);
        swap(playerInfos[player2].aiInfo, playerInfos[player1].aiInfo);
        swap(playerInfos[player2].isHost, playerInfos[player1].isHost);
    }
    // Change ids of network players (if any). Get both first!
    GameServerPlayer* newPlayer = GetNetworkPlayer(player2);
    GameServerPlayer* oldPlayer = GetNetworkPlayer(player1);
    if(newPlayer)
        newPlayer->playerId = player1;
    if(oldPlayer)
        oldPlayer->playerId = player2;
    SendToAll(GameMessage_Player_Swap(player1, player2));
    const auto pSwap = std::make_pair(player1, player2);
    for(GameServerPlayer& player : networkPlayers)
    {
        if(!player.isActive())
            continue;
        player.getPendingSwaps().push_back(pSwap);
    }
}

GameServerPlayer* GameServer::GetNetworkPlayer(unsigned playerId)
{
    for(GameServerPlayer& player : networkPlayers)
    {
        if(player.playerId == playerId)
            return &player;
    }
    return nullptr;
}

void GameServer::SetPaused(bool paused)
{
    if(framesinfo.isPaused == paused)
        return;
    framesinfo.isPaused = paused;
    SendToAll(GameMessage_Pause(framesinfo.isPaused));
    for(GameServerPlayer& player : networkPlayers)
        player.setNotLagging();
}

JoinPlayerInfo& GameServer::GetJoinPlayer(unsigned playerIdx)
{
    return playerInfos.at(playerIdx);
}

bool GameServer::IsHost(unsigned playerIdx) const
{
    return playerIdx < playerInfos.size() && playerInfos[playerIdx].isHost;
}

int GameServer::GetTargetPlayer(const GameMessageWithPlayer& msg)
{
    if(msg.player != 0xFF)
    {
        if(msg.player < playerInfos.size() && (msg.player == msg.senderPlayerID || IsHost(msg.senderPlayerID)))
        {
            GameServerPlayer* networkPlayer = GetNetworkPlayer(msg.senderPlayerID);
            if(networkPlayer->isActive())
                return msg.player;
            unsigned result = msg.player;
            // Apply pending swaps
            for(auto& pSwap : networkPlayer->getPendingSwaps()) //-V522
            {
                if(pSwap.first == result)
                    result = pSwap.second;
                else if(pSwap.second == result)
                    result = pSwap.first;
            }
            return result;
        }
    } else if(msg.senderPlayerID < playerInfos.size())
        return msg.senderPlayerID;
    return -1;
}
