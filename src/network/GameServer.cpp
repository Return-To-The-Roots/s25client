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

#include "rttrDefines.h" // IWYU pragma: keep
#include "GameServer.h"
#include "Debug.h"
#include "GameManager.h"
#include "GameMessage.h"
#include "GameMessage_GameCommand.h"
#include "GameServerPlayer.h"
#include "GlobalGameSettings.h"
#include "Loader.h"
#include "RTTR_Version.h"
#include "RttrConfig.h"
#include "Savegame.h"
#include "Settings.h"
#include "ai/AIPlayer.h"
#include "drivers/VideoDriverWrapper.h"
#include "files.h"
#include "helpers/Deleter.h"
#include "helpers/containerUtils.h"
#include "ingameWindows/iwDirectIPCreate.h"
#include "network/GameClient.h"
#include "network/GameMessages.h"
#include "ogl/glArchivItem_Bitmap.h"
#include "ogl/glArchivItem_Map.h"
#include "gameTypes/LanGameInfo.h"
#include "gameData/GameConsts.h"
#include "gameData/LanDiscoveryCfg.h"
#include "liblobby/LobbyClient.h"
#include "libsiedler2/ArchivItem_Map_Header.h"
#include "libsiedler2/prototypen.h"
#include "libutil/SocketSet.h"
#include "libutil/colors.h"
#include "libutil/ucString.h"
#include <boost/bind.hpp>
#include <boost/filesystem.hpp>
#include <boost/foreach.hpp>
#include <boost/nowide/fstream.hpp>
#include <iomanip>

inline std::ostream& operator<<(std::ostream& os, const AsyncChecksum& checksum)
{
    return os << "RandCS = " << checksum.randChecksum << ",\tobjects/ID = " << checksum.objCt << "/" << checksum.objIdCt
              << ",\tevents/ID = " << checksum.eventCt << "/" << checksum.evInstanceCt;
}

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
    playercount = 0;
    gamename.clear();
    password.clear();
    port = 0;
    ipv6 = false;
    use_upnp = false;
}

GameServer::CountDown::CountDown() : isActive(false), remainingSecs(0), lasttime(0) {}

void GameServer::CountDown::Start(unsigned timeInSec, unsigned curTime)
{
    isActive = true;
    remainingSecs = timeInSec;
    lasttime = curTime;
}

void GameServer::CountDown::Stop()
{
    isActive = false;
}

bool GameServer::CountDown::Update(unsigned curTime)
{
    RTTR_Assert(isActive);
    // Check if 1s has passed
    if(curTime - lasttime < 1000)
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
GameServer::GameServer() : skiptogf(0), status(SS_STOPPED), currentGF(0), lanAnnouncer(LAN_DISCOVERY_CFG) {}

///////////////////////////////////////////////////////////////////////////////
//
GameServer::~GameServer()
{
    Stop();
}

///////////////////////////////////////////////////////////////////////////////
// Spiel hosten
bool GameServer::TryToStart(const CreateServerInfo& csi, const std::string& map_path, const MapType map_type)
{
    Stop();

    // Name, Password und Kartenname kopieren
    config.gamename = csi.gamename;
    config.password = csi.password;
    config.servertype = csi.type;
    config.port = csi.port;
    config.ipv6 = csi.ipv6;
    config.use_upnp = csi.use_upnp;
    mapinfo.type = map_type;
    mapinfo.filepath = map_path;

    // Maps, Random-Maps, Savegames - Header laden und relevante Informationen rausschreiben (Map-Titel, Spieleranzahl)
    switch(mapinfo.type)
    {
        default:
            LOG.write("GameServer::Start: ERROR: Map-Type %u not supported!\n") % mapinfo.type;
            return false;
        // Altes S2-Mapformat von BB
        case MAPTYPE_OLDMAP:
        {
            libsiedler2::Archiv map;

            // Karteninformationen laden
            if(libsiedler2::loader::LoadMAP(mapinfo.filepath, map, true) != 0)
            {
                LOG.write("GameServer::Start: ERROR: Map \"%s\", couldn't load header!\n") % mapinfo.filepath;
                return false;
            }
            const libsiedler2::ArchivItem_Map_Header& header = checkedCast<const glArchivItem_Map*>(map.get(0))->getHeader();

            config.playercount = header.getNumPlayers();
            mapinfo.title = cvStringToUTF8(header.getName());
        }
        break;
        // Gespeichertes Spiel
        case MAPTYPE_SAVEGAME:
        {
            Savegame save;

            if(!save.Load(mapinfo.filepath, true, false))
                return false;

            // Spieleranzahl
            config.playercount = save.GetNumPlayers();
            mapinfo.title = save.GetMapName();
        }
        break;
    }

    status = SS_CREATING_LOBBY;
    // Von Lobby abhängig? Dann der Bescheid sagen und auf eine Antwort warten, dass wir den Server
    // erstellen dürfen
    if(config.servertype == ServerType::LOBBY)
    {
        LOBBYCLIENT.AddServer(config.gamename, mapinfo.title, (config.password.length() != 0), config.port);
        return true;
    } else
        // ansonsten können wir sofort starten
        return Start();
}

bool GameServer::Start()
{
    RTTR_Assert(status == SS_CREATING_LOBBY);

    if(!mapinfo.mapData.CompressFromFile(mapinfo.filepath, &mapinfo.mapChecksum))
        return false;

    std::string luaFilePath = mapinfo.filepath.substr(0, mapinfo.filepath.length() - 3) + "lua";
    if(bfs::exists(luaFilePath))
    {
        if(!mapinfo.luaData.CompressFromFile(luaFilePath, &mapinfo.luaChecksum))
            return false;
        mapinfo.luaFilepath = luaFilePath;
    } else
        RTTR_Assert(mapinfo.luaFilepath.empty() && mapinfo.luaChecksum == 0);

    playerInfos.resize(config.playercount);
    ai_players.resize(config.playercount);

    bool host_found = false;

    //// Spieler 0 erstmal der Host
    switch(mapinfo.type)
    {
        default: break;

        case MAPTYPE_OLDMAP:
        {
            // Host bei normalen Spieler der erste Spieler
            playerInfos[0].isHost = true;

            ggs_.LoadSettings();
        }
        break;
        case MAPTYPE_SAVEGAME:
        {
            Savegame save;
            if(!save.Load(mapinfo.filepath, true, false))
                return false;

            // Bei Savegames die Originalspieldaten noch mit auslesen
            for(unsigned i = 0; i < playerInfos.size(); ++i)
            {
                playerInfos[i] = JoinPlayerInfo(save.GetPlayer(i));
                if(playerInfos[i].ps == PS_OCCUPIED)
                {
                    // If it was a human we make it free, so someone can join
                    playerInfos[i].ps = PS_FREE;
                    // First player becomes the host
                    if(!host_found)
                    {
                        playerInfos[i].isHost = true;
                        host_found = true;
                    }
                }
                playerInfos[i].InitRating();
            }

            ggs_ = save.ggs;
        }
        break;
    }

    // ab in die Konfiguration
    status = SS_CONFIG;

    // und das socket in listen-modus schicken
    if(!serversocket.Listen(config.port, config.ipv6, config.use_upnp))
    {
        LOG.write("GameServer::Start: ERROR: Listening on port %d failed!\n") % config.port;
        LOG.writeLastError("Fehler");
        return false;
    }

    // Zu sich selbst connecten als Host
    if(!GAMECLIENT.Connect("localhost", config.password, config.servertype, config.port, true, config.ipv6))
        return false;

    if(config.servertype == ServerType::LAN)
        lanAnnouncer.Start();
    AnnounceStatusChange();

    return true;
}

unsigned GameServer::GetNumFilledSlots() const
{
    unsigned numFilled = 0;
    BOOST_FOREACH(const JoinPlayerInfo& player, playerInfos)
    {
        if(player.ps != PS_FREE)
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
        info.version = RTTR_Version::GetReadableVersion();
        info.revision = RTTR_Version::GetRevision();
        Serializer ser;
        info.Serialize(ser);
        lanAnnouncer.SetPayload(ser.GetData(), ser.GetLength());
    } else if(config.servertype == ServerType::LOBBY)
    {
        LOBBYCLIENT.UpdateServerNumPlayers(GetNumFilledSlots(), playerInfos.size());
    }
}

///////////////////////////////////////////////////////////////////////////////
// Hauptschleife
void GameServer::Run()
{
    if(status == SS_STOPPED)
        return;

    // auf tote Clients prüfen
    if(status != SS_CREATING_LOBBY)
        ClientWatchDog();

    // auf neue Clients warten
    if(status == SS_CONFIG)
        WaitForClients();
    else if(status == SS_GAME)
        ExecuteGameFrame();

    // post zustellen
    FillPlayerQueues();

    if(countdown.IsActive())
    {
        // countdown erzeugen
        if(countdown.Update(VIDEODRIVER.GetTickCount()))
        {
            // nun echt starten
            if(!countdown.IsActive())
            {
                if(!StartGame())
                {
                    GAMEMANAGER.ShowMenu();
                    return;
                }
            } else
                SendToAll(GameMessage_Countdown(countdown.GetRemainingSecs()));
        }
    }

    // queues abarbeiten
    BOOST_FOREACH(GameServerPlayer& player, networkPlayers)
    {
        // maximal 10 Pakete verschicken
        player.sendMsgs(10);
        player.executeMsgs(*this);
    }

    for(std::vector<GameServerPlayer>::iterator it = networkPlayers.begin(); it != networkPlayers.end();)
    {
        if(!it->socket.isValid())
            it = networkPlayers.erase(it);
        else
            ++it;
    }

    lanAnnouncer.Run();
}

///////////////////////////////////////////////////////////////////////////////
// stoppt den server
void GameServer::Stop()
{
    if(status == SS_STOPPED)
        return;

    // player verabschieden
    playerInfos.clear();
    networkPlayers.clear();

    // aufräumen
    framesinfo.Clear();
    config.Clear();
    mapinfo.Clear();
    countdown.Stop();

    // KI-Player zerstören
    for(unsigned i = 0; i < ai_players.size(); ++i)
        delete ai_players[i];
    ai_players.clear();

    // laden dicht machen
    serversocket.Close();
    // clear jump target
    skiptogf = 0;

    // clear async logs
    asyncLogs.clear();

    lanAnnouncer.Stop();

    if(LOBBYCLIENT.IsLoggedIn()) // steht die Lobbyverbindung noch?
        LOBBYCLIENT.DeleteServer();

    // status
    status = SS_STOPPED;
    LOG.write("server state changed to stop\n");
}

/**
 *  startet das Spiel.
 */
bool GameServer::StartGame()
{
    lanAnnouncer.Stop();

    // Bei Savegames wird der Startwert von den Clients aus der Datei gelesen!
    unsigned random_init = (mapinfo.type == MAPTYPE_SAVEGAME) ? 0 : VIDEODRIVER.GetTickCount();

    // Höchsten Ping ermitteln
    unsigned highest_ping = 0;
    BOOST_FOREACH(const JoinPlayerInfo& player, playerInfos)
    {
        if(player.ps == PS_OCCUPIED)
        {
            if(player.ping > highest_ping)
                highest_ping = player.ping;
        }
    }

    framesinfo.gfLenghtNew = framesinfo.gfLenghtNew2 = framesinfo.gf_length = SPEED_GF_LENGTHS[ggs_.speed];

    // NetworkFrame-Länge bestimmen, je schlechter (also höher) die Pings, desto länger auch die Framelänge
    unsigned i = 1;
    for(; i < 20; ++i)
    {
        if(i * framesinfo.gf_length > highest_ping + 200)
            break;
    }

    framesinfo.nwf_length = i;

    LOG.write("SERVER: Using gameframe length of %dms\n") % framesinfo.gf_length;
    LOG.write("SERVER: Using networkframe length of %u GFs (%ums)\n") % framesinfo.nwf_length
      % (framesinfo.nwf_length * framesinfo.gf_length);

    // Spielstart allen mitteilen
    SendToAll(GameMessage_Server_Start(random_init, framesinfo.nwf_length));
    LOG.writeToFile("SERVER >>> BROADCAST: NMS_SERVER_START(%d)\n") % random_init;

    framesinfo.lastTime = VIDEODRIVER.GetTickCount();

    try
    {
        // GameClient soll erstmal starten, damit wir von ihm die benötigten Daten für die KIs bekommen
        GAMECLIENT.StartGame(random_init);
    } catch(SerializedGameData::Error& error)
    {
        LOG.write("Error when loading game: %s\n") % error.what();
        return false;
    }

    // Init current GF
    currentGF = GAMECLIENT.GetGFNumber();

    // Erste KI-Nachrichten schicken
    for(unsigned i = 0; i < playerInfos.size(); ++i)
    {
        if(playerInfos[i].ps == PS_AI)
        {
            ai_players[i] = GAMECLIENT.CreateAIPlayer(i, playerInfos[i].aiInfo);
            SendNothingNC(i);
        }
    }

    LOG.writeToFile("SERVER >>> BROADCAST: NMS_NWF_DONE\n");

    // Spielstart allen mitteilen
    SendToAll(GameMessage_Server_NWFDone(currentGF, framesinfo.gf_length, true));

    // ab ins game wechseln
    status = SS_GAME;

    return true;
}

/**
 *  Nachricht an Alle
 */
void GameServer::SendToAll(const GameMessage& msg)
{
    BOOST_FOREACH(GameServerPlayer& player, networkPlayers)
    {
        // ist der Slot Belegt, dann Nachricht senden
        if(player.isConnected())
            player.sendMsgAsync(msg.clone());
    }
}

void GameServer::KickPlayer(uint8_t playerId, KickReason cause)
{
    if(playerId >= playerInfos.size())
        return;
    JoinPlayerInfo& playerInfo = playerInfos[playerId];
    GameServerPlayer* player = GetNetworkPlayer(playerId);
    if(player)
        player->closeConnection(true);
    // Non-existing or connecting player
    if(!playerInfo.isUsed())
        return;
    playerInfo.ps = PS_FREE;

    SendToAll(GameMessage_Player_Kicked(playerId, cause));

    // If we are ingame, replace by KI
    if(status == SS_GAME)
    {
        playerInfo.ps = PS_AI;
        playerInfo.aiInfo.type = AI::DUMMY;
        playerInfo.aiInfo.level = AI::MEDIUM;
        ai_players[playerId] = GAMECLIENT.CreateAIPlayer(playerId, playerInfo.aiInfo);
        SendNothingNC(playerId);
    }

    AnnounceStatusChange();
    LOG.writeToFile("SERVER >>> BROADCAST: NMS_PLAYERKICKED(%d,%d)\n") % unsigned(playerId) % unsigned(cause);
}

///////////////////////////////////////////////////////////////////////////////
// testet, ob in der Verbindungswarteschlange Clients auf Verbindung warten
void GameServer::ClientWatchDog()
{
    SocketSet set;
    set.Clear();

    // sockets zum set hinzufügen
    BOOST_FOREACH(GameServerPlayer& player, networkPlayers)
        set.Add(player.socket);

    // auf fehler prüfen
    if(set.Select(0, 2) > 0)
    {
        BOOST_FOREACH(const GameServerPlayer& player, networkPlayers)
        {
            if(set.InSet(player.socket))
            {
                LOG.write(_("SERVER: Error on socket of player %1%, bye bye!\n")) % player.playerId;
                KickPlayer(player.playerId, NP_CONNECTIONLOST);
            }
        }
    }

    BOOST_FOREACH(GameServerPlayer& player, networkPlayers)
    {
        if(player.hasConnectTimedOut())
        {
            LOG.write(_("SERVER: Reserved slot %1% freed due to timeout\n")) % player.playerId;
            KickPlayer(player.playerId);
        } else
            player.doPing();
    }
}

void GameServer::ExecuteGameFrame()
{
    RTTR_Assert(status == SS_GAME);

    if(framesinfo.isPaused)
        return;

    unsigned currentTime = VIDEODRIVER.GetTickCount();

    // prüfen ob GF vergangen
    if(currentTime - framesinfo.lastTime >= framesinfo.gf_length || skiptogf > currentGF)
    {
        // NWF vergangen?
        if(currentGF % framesinfo.nwf_length == 0)
        {
            if(CheckForLaggingPlayers())
            {
                // Check for kicking every second
                static unsigned lastLagKickTime = 0;
                if(VIDEODRIVER.GetTickCount() - lastLagKickTime >= 1000)
                {
                    lastLagKickTime = VIDEODRIVER.GetTickCount();
                    CheckAndKickLaggingPlayers();
                }
            } else
            {
                ExecuteNWF(currentTime);
                // Execute RunGF AFTER the NWF is send.
                RunGF(true);
            }
        } else
        {
            RunGF(false);
            // Diesen Zeitpunkt merken
            framesinfo.lastTime = currentTime;
        }
    }
}

void GameServer::RunGF(bool isNWF)
{
    // KIs ausführen
    BOOST_FOREACH(AIPlayer* ai_player, ai_players)
    {
        if(ai_player)
            ai_player->RunGF(currentGF, isNWF);
    }
    ++currentGF;
}

void GameServer::ExecuteNWF(const unsigned /*currentTime*/)
{
    // Advance lastExecutedTime by the GF length.
    // This is not really the last executed time, but if we waited (or laggt) we can catch up a bit by executing the next GF earlier
    framesinfo.lastTime += framesinfo.gf_length;

    /// Handle AI
    BOOST_FOREACH(AIPlayer* ai, ai_players)
    {
        if(!ai)
            continue;
        SendToAll(GameMessage_GameCommand(ai->GetPlayerId(), AsyncChecksum(), ai->GetGameCommands()));
        ai->FetchGameCommands();
    }

    // Check for asyncs
    if(CheckForAsync())
    {
        // Pause game
        RTTR_Assert(!framesinfo.isPaused);
        SetPaused(true);

        // Notify players
        std::vector<unsigned> checksumHashes;
        BOOST_FOREACH(const GameServerPlayer& player, networkPlayers)
            checksumHashes.push_back(player.checksumOfNextNWF.front().getHash());
        SendToAll(GameMessage_Server_Async(checksumHashes));

        // Request async logs
        BOOST_FOREACH(GameServerPlayer& player, networkPlayers)
        {
            asyncLogs.push_back(AsyncLog(player.playerId, player.checksumOfNextNWF.front()));
            player.sendMsgAsync(new GameMessage_GetAsyncLog());
        }
    }
    // Remove first (current) msg
    BOOST_FOREACH(GameServerPlayer& player, networkPlayers)
    {
        player.checksumOfNextNWF.pop();
        RTTR_Assert(player.checksumOfNextNWF.size() <= 1); // At most 1 additional GC-Message, otherwise the client skipped a NWF
    }

    if(framesinfo.gfLenghtNew != framesinfo.gf_length)
    {
        unsigned oldGfLen = framesinfo.gf_length;
        unsigned oldnNwfLen = framesinfo.nwf_length;
        framesinfo.ApplyNewGFLength();

        LOG.write(_("SERVER: At GF %1%: Speed changed from %2% to %3%. NWF %4% to %5%\n")) % currentGF % oldGfLen % framesinfo.gf_length
          % oldnNwfLen % framesinfo.nwf_length;
    }

    framesinfo.gfLenghtNew = framesinfo.gfLenghtNew2;

    SendToAll(GameMessage_Server_NWFDone(currentGF, framesinfo.gfLenghtNew));
}

bool GameServer::CheckForAsync()
{
    if(networkPlayers.empty())
        return false;
    bool isAsync = false;
    const AsyncChecksum& refChecksum = networkPlayers.front().checksumOfNextNWF.front();
    BOOST_FOREACH(const GameServerPlayer& player, networkPlayers)
    {
        RTTR_Assert(!player.checksumOfNextNWF.empty()); // Players should not be lagging at this point
        const AsyncChecksum& curChecksum = player.checksumOfNextNWF.front();

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
    BOOST_FOREACH(const GameServerPlayer& player, networkPlayers)
    {
        const unsigned timeOut = player.getLagTimeOut();
        if(timeOut == 0)
            KickPlayer(player.playerId, NP_PINGTIMEOUT);
        else if(timeOut <= 30
                && (timeOut % 5 == 0 || timeOut < 5)) // Notify every 5s if max 30s are remaining, if less than 5s notify every second
            LOG.write(_("SERVER: Kicking player %1% in %2% seconds\n")) % player.playerId % timeOut;
    }
}

bool GameServer::CheckForLaggingPlayers()
{
    bool isLagging = false;
    BOOST_FOREACH(GameServerPlayer& player, networkPlayers)
    {
        if(player.checksumOfNextNWF.empty())
        {
            player.setLagging();
            isLagging = true;
        }
    }
    return isLagging;
}

/**
 *  Sendet ein NC-Paket ohne Befehle.
 */
void GameServer::SendNothingNC(const unsigned id)
{
    SendToAll(GameMessage_GameCommand(id, AsyncChecksum(), std::vector<gc::GameCommandPtr>()));
}

///////////////////////////////////////////////////////////////////////////////
// testet, ob in der Verbindungswarteschlange Clients auf Verbindung warten
void GameServer::WaitForClients()
{
    SocketSet set;

    set.Add(serversocket);
    if(set.Select(0, 0) > 0)
    {
        if(set.InSet(serversocket))
        {
            Socket socket = serversocket.Accept();

            // Verbindung annehmen
            if(!socket.isValid())
                return;

            unsigned newPlayerId = 0xFFFFFFFF;
            // Geeigneten Platz suchen
            for(unsigned playerId = 0; playerId < playerInfos.size(); ++playerId)
            {
                if(playerInfos[playerId].ps == PS_FREE && !GetNetworkPlayer(playerId))
                {
                    networkPlayers.push_back(GameServerPlayer(playerId, socket));
                    newPlayerId = playerId;
                    // LOG.write(("new socket, about to tell him about his playerId: %i \n",playerId);
                    // schleife beenden
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
        BOOST_FOREACH(const GameServerPlayer& player, networkPlayers)
            set.Add(player.socket);

        msgReceived = false;

        // ist eines der Sockets im Set lesbar?
        if(set.Select(0, 0) > 0)
        {
            BOOST_FOREACH(GameServerPlayer& player, networkPlayers)
            {
                if(set.InSet(player.socket))
                {
                    // nachricht empfangen
                    if(!player.receiveMsgs())
                    {
                        LOG.write(_("SERVER: Receiving Message for player %1% failed, kicking...\n")) % player.playerId;
                        KickPlayer(player.playerId, NP_CONNECTIONLOST);
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
    GameServerPlayer* player = GetNetworkPlayer(msg.senderPlayerID);
    if(!player)
        return true;

    int typeok = 0;
    if(msg.type != config.servertype)
        typeok = 1;
    else if(msg.revision != RTTR_Version::GetRevision())
        typeok = 2;

    player->sendMsgAsync(new GameMessage_Server_TypeOK(typeok));

    if(typeok != 0)
        KickPlayer(msg.senderPlayerID, NP_CONNECTIONLOST);
    return true;
}

/**
 *  Server-Passwort-Nachricht
 */
bool GameServer::OnGameMessage(const GameMessage_Server_Password& msg)
{
    GameServerPlayer* player = GetNetworkPlayer(msg.senderPlayerID);
    if(!player)
        return true;

    std::string passwordok = (config.password == msg.password ? "true" : "false");

    player->sendMsgAsync(new GameMessage_Server_Password(passwordok));

    if(passwordok == "false")
        KickPlayer(msg.senderPlayerID, NP_WRONGPASSWORD);
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
    // Can't do this. Have to have a joined player
    if(msg.ps == PS_OCCUPIED)
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
            KickPlayer(playerID, NP_NOCAUSE);

        if(mapinfo.type == MAPTYPE_SAVEGAME)
        {
            // For savegames we cannot set anyone on a locked slot as the player does not exist on the map
            if(player.ps != PS_LOCKED)
            {
                // And we don't lock!
                player.ps = msg.ps == PS_LOCKED ? PS_FREE : msg.ps;
                player.aiInfo = msg.aiInfo;
            }
        } else
        {
            player.ps = msg.ps;
            player.aiInfo = msg.aiInfo;
        }
        if(player.ps == PS_FREE && config.servertype == ServerType::LOCAL)
        {
            player.ps = PS_AI;
            player.aiInfo = AI::Info(AI::DEFAULT);
        }
    }
    // Even when nothing changed we send the data because the other players might have expected a change

    SendToAll(GameMessage_Player_State(playerID, player.ps, player.aiInfo));

    if(player.ps == PS_AI)
    {
        player.SetAIName(playerID);
        SendToAll(GameMessage_Player_Name(playerID, player.name));
    }
    // If slot is filled, check current color
    if(player.isUsed())
        CheckAndSetColor(playerID, player.color);
    if(oldPs != player.ps)
        player.isReady = (player.ps == PS_AI);
    SendToAll(GameMessage_Player_Ready(playerID, player.isReady));
    AnnounceStatusChange();
    return true;
}

///////////////////////////////////////////////////////////////////////////////
// Spielername
bool GameServer::OnGameMessage(const GameMessage_Player_Name& msg)
{
    int playerID = GetTargetPlayer(msg);
    if(playerID < 0)
        return true;

    LOG.writeToFile("CLIENT%d >>> SERVER: NMS_PLAYER_NAME(%s)\n") % playerID % msg.playername;

    playerInfos[playerID].name = msg.playername;
    SendToAll(GameMessage_Player_Name(playerID, msg.playername));

    return true;
}

///////////////////////////////////////////////////////////////////////////////
// Nation weiterwechseln
bool GameServer::OnGameMessage(const GameMessage_Player_Nation& msg)
{
    int playerID = GetTargetPlayer(msg);
    if(playerID < 0)
        return true;

    playerInfos[playerID].nation = msg.nation;

    SendToAll(GameMessage_Player_Nation(playerID, msg.nation));
    return true;
}

///////////////////////////////////////////////////////////////////////////////
// Team weiterwechseln
bool GameServer::OnGameMessage(const GameMessage_Player_Team& msg)
{
    int playerID = GetTargetPlayer(msg);
    if(playerID < 0)
        return true;

    playerInfos[playerID].team = msg.team;

    SendToAll(GameMessage_Player_Team(playerID, msg.team));
    return true;
}

///////////////////////////////////////////////////////////////////////////////
// Farbe weiterwechseln
bool GameServer::OnGameMessage(const GameMessage_Player_Color& msg)
{
    int playerID = GetTargetPlayer(msg);
    if(playerID < 0)
        return true;

    CheckAndSetColor(playerID, msg.color);
    return true;
}

/**
 *  Spielerstatus wechseln
 */
bool GameServer::OnGameMessage(const GameMessage_Player_Ready& msg)
{
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
    GameServerPlayer* player = GetNetworkPlayer(msg.senderPlayerID);
    if(!player)
        return true;

    if(msg.requestInfo)
    {
        player->sendMsgAsync(new GameMessage_Map_Info(bfs::path(mapinfo.filepath).filename().string(), mapinfo.type, mapinfo.mapData.length,
                                                      mapinfo.mapData.data.size(), mapinfo.luaData.length, mapinfo.luaData.data.size()));
    } else if(player->mapDataSent)
    {
        // Don't send again
        KickPlayer(msg.senderPlayerID, NP_INVALIDMSG);
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
        RTTR_Assert(mapinfo.luaData.data.empty() == (mapinfo.luaData.length == 0));
        curPos = 0;
        remainingSize = mapinfo.luaData.data.size();
        while(remainingSize)
        {
            unsigned chunkSize = std::min(MAP_PART_SIZE, remainingSize);

            player->sendMsgAsync(new GameMessage_Map_Data(false, curPos, &mapinfo.luaData.data[curPos], chunkSize));
            curPos += chunkSize;
            remainingSize -= chunkSize;
        }
        player->mapDataSent = true;
    }
    return true;
}

bool GameServer::OnGameMessage(const GameMessage_Map_Checksum& msg)
{
    GameServerPlayer* player = GetNetworkPlayer(msg.senderPlayerID);
    if(!player)
        return true;

    bool checksumok = (msg.mapChecksum == mapinfo.mapChecksum && msg.luaChecksum == mapinfo.luaChecksum);

    LOG.writeToFile("CLIENT%d >>> SERVER: NMS_MAP_CHECKSUM(%u) expected: %u, ok: %s\n") % unsigned(msg.senderPlayerID) % msg.mapChecksum
      % mapinfo.mapChecksum % (checksumok ? "yes" : "no");

    // Send response. If map data was not sent yet, the client may retry
    player->sendMsgAsync(new GameMessage_Map_ChecksumOK(checksumok, !player->mapDataSent));

    LOG.writeToFile("SERVER >>> CLIENT%d: NMS_MAP_CHECKSUM(%d)\n") % unsigned(msg.senderPlayerID) % checksumok;

    if(!checksumok)
    {
        if(player->mapDataSent)
            KickPlayer(msg.senderPlayerID, NP_WRONGCHECKSUM);
    } else
    {
        JoinPlayerInfo& playerInfo = playerInfos[msg.senderPlayerID];
        // den anderen Spielern mitteilen das wir einen neuen haben
        SendToAll(GameMessage_Player_New(msg.senderPlayerID, playerInfo.name));

        LOG.writeToFile("SERVER >>> BROADCAST: NMS_PLAYER_NEW(%d, %s)\n") % unsigned(msg.senderPlayerID) % playerInfo.name;

        // belegt markieren
        playerInfo.ps = PS_OCCUPIED;
        player->setConnected();

        // Servername senden
        player->sendMsgAsync(new GameMessage_Server_Name(config.gamename));

        // Spielerliste senden
        player->sendMsgAsync(new GameMessage_Player_List(playerInfos));

        // Assign unique color
        CheckAndSetColor(msg.senderPlayerID, playerInfo.color);

        // GGS senden
        player->sendMsgAsync(new GameMessage_GGSChange(ggs_));

        AnnounceStatusChange();
    }
    return true;
}

// speed change message
bool GameServer::OnGameMessage(const GameMessage_Speed& msg)
{
    framesinfo.gfLenghtNew2 = msg.gf_length;
    return true;
}

bool GameServer::OnGameMessage(const GameMessage_GameCommand& msg)
{
    GameServerPlayer* player = GetNetworkPlayer(msg.senderPlayerID);
    if(!player)
        return true;
    // LOG.writeToFile("SERVER <<< GC %u\n") % unsigned(msg.senderPlayerID);

    // Save and broadcast command
    player->checksumOfNextNWF.push(msg.gcs.checksum);
    player->setNotLagging();
    SendToAll(GameMessage_GameCommand(msg.senderPlayerID, msg.gcs.checksum, msg.gcs.gcs));
    return true;
}

bool GameServer::OnGameMessage(const GameMessage_AsyncLog& msg)
{
    bool foundPlayer = false;
    BOOST_FOREACH(AsyncLog& log, asyncLogs)
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
            LOG.write(_("Received async logs from %1% (%2% entries).\n")) % unsigned(log.playerId) % log.randEntries.size();
            log.done = true;
        }
    }
    if(!foundPlayer)
    {
        LOG.write(_("Received async log from %1%, but did not expect it!\n")) % unsigned(msg.senderPlayerID);
        return true;
    }

    // Check if we have all logs
    BOOST_FOREACH(const AsyncLog& log, asyncLogs)
    {
        if(!log.done)
            return true;
    }

    LOG.write(_("Async logs received completely.\n"));

    std::string asyncFilePath = SaveAsyncLog();
    if(!asyncFilePath.empty())
        SendAsyncLog(asyncFilePath);

    // Kick all players that have a different checksum from the host
    AsyncChecksum hostChecksum;
    BOOST_FOREACH(const AsyncLog& log, asyncLogs)
    {
        if(playerInfos.at(log.playerId).isHost)
        {
            hostChecksum = log.checksum;
            break;
        }
    }
    BOOST_FOREACH(const AsyncLog& log, asyncLogs)
    {
        if(log.checksum != hostChecksum)
            KickPlayer(log.playerId, NP_ASYNC);
    }
    return true;
}

bool GameServer::OnGameMessage(const GameMessage_RemoveLua& msg)
{
    if(status != SS_CONFIG || !IsHost(msg.senderPlayerID))
        return true;
    mapinfo.luaFilepath.clear();
    mapinfo.luaData.Clear();
    mapinfo.luaChecksum = 0;
    SendToAll(msg);
    return true;
}

bool GameServer::OnGameMessage(const GameMessage_Countdown& msg)
{
    if(status != SS_CONFIG)
        return true;

    NetworkPlayer* nwPlayer = GetNetworkPlayer(msg.senderPlayerID);
    if(!nwPlayer || !IsHost(msg.senderPlayerID) || countdown.IsActive())
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
            countdown.Start(msg.countdown, VIDEODRIVER.GetTickCount());
            SendToAll(GameMessage_Countdown(countdown.GetRemainingSecs()));
            LOG.writeToFile("SERVER >>> Countdown started(%d)\n") % countdown.GetRemainingSecs();
        } else if(!StartGame())
            GAMEMANAGER.ShowMenu(); // TODO: Remove access to GAMEMANAGER
    }

    return true;
}

bool GameServer::OnGameMessage(const GameMessage_CancelCountdown& msg)
{
    if(status != SS_CONFIG)
        return true;

    if(IsHost(msg.senderPlayerID))
        CancelCountdown();
    return true;
}

bool GameServer::OnGameMessage(const GameMessage_GGSChange& msg)
{
    if(status == SS_CONFIG && IsHost(msg.senderPlayerID))
    {
        ggs_ = msg.ggs;
        SendToAll(msg);
    }
    return true;
}

void GameServer::CancelCountdown()
{
    if(!countdown.IsActive())
        return;
    // Countdown-Stop allen mitteilen
    countdown.Stop();
    SendToAll(GameMessage_CancelCountdown());
    LOG.writeToFile("SERVER >>> BROADCAST: NMS_SERVER_CANCELCOUNTDOWN\n");
}

bool GameServer::ArePlayersReady() const
{
    // Alle Spieler da?
    BOOST_FOREACH(const JoinPlayerInfo& player, playerInfos)
    {
        // noch nicht alle spieler da -> feierabend!
        if(player.ps == PS_FREE)
            return false;
        else if(player.isHuman() && !player.isReady)
            return false;
    }

    std::set<unsigned> takenColors;

    // Check all players have different colors
    BOOST_FOREACH(const JoinPlayerInfo& player, playerInfos)
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

std::string GameServer::SaveAsyncLog()
{
    // Get the highest common counter number and start from there (remove all others)
    unsigned maxCtr = 0;
    BOOST_FOREACH(const AsyncLog& log, asyncLogs)
    {
        if(!log.randEntries.empty() && log.randEntries[0].counter > maxCtr)
            maxCtr = log.randEntries[0].counter;
    }
    // Number of entries = max(asyncLogs[0..n].randEntries.size())
    unsigned numEntries = 0;
    BOOST_FOREACH(AsyncLog& log, asyncLogs)
    {
        std::vector<RandomEntry>::iterator it =
          std::find_if(log.randEntries.begin(), log.randEntries.end(), boost::bind(&RandomEntry::counter, _1) == maxCtr);
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
        const RandomEntry& refEntry = asyncLogs[0].randEntries[i];
        BOOST_FOREACH(const AsyncLog& log, asyncLogs)
        {
            if(i >= log.randEntries.size())
            {
                isIdentical = false;
                break;
            }
            const RandomEntry& curEntry = log.randEntries[i];
            if(curEntry.max != refEntry.max || curEntry.rngState != refEntry.rngState || curEntry.obj_id != refEntry.obj_id)
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

    std::string filePath =
      RTTRCONFIG.ExpandPath(FILE_PATHS[47]) + "/" + s25util::Time::FormatTime("async_%Y-%m-%d_%H-%i-%s") + "Server.log";

    // open async log
    bnw::ofstream file(filePath);

    if(file)
    {
        file << "Map: " << mapinfo.title << std::endl;
        file << std::setfill(' ');
        BOOST_FOREACH(const AsyncLog& log, asyncLogs)
        {
            const JoinPlayerInfo& plInfo = playerInfos.at(log.playerId);
            file << "Player " << std::setw(2) << unsigned(log.playerId) << (plInfo.isHost ? '#' : ' ') << "\t\"" << plInfo.name << '"'
                 << std::endl;
            file << "System info: " << log.addData << std::endl;
            file << "\tChecksum: " << std::setw(0) << log.checksum << std::endl;
        }
        BOOST_FOREACH(const AsyncLog& log, asyncLogs)
            file << "Checksum " << std::setw(2) << unsigned(log.playerId) << std::setw(0) << ": " << log.checksum << std::endl;

        // print identical lines, they help in tracing the bug
        for(unsigned i = 0; i < numIdentical; i++)
            file << "[ I ]: " << asyncLogs[0].randEntries[i] << "\n";
        for(unsigned i = numIdentical; i < numEntries; i++)
        {
            BOOST_FOREACH(const AsyncLog& log, asyncLogs)
            {
                if(i < log.randEntries.size())
                    file << "[C" << std::setw(2) << unsigned(log.playerId) << std::setw(0) << "]: " << log.randEntries[i] << '\n';
            }
        }

        LOG.write(_("Async log saved at \"%s\"\n")) % filePath;
        return filePath;
    } else
    {
        LOG.write(_("Failed to save async log at \"%s\"\n")) % filePath;
        return "";
    }
}

void GameServer::SendAsyncLog(const std::string& asyncLogFilePath)
{
    if(SETTINGS.global.submit_debug_data == 1
#ifdef _WIN32
       || (MessageBoxW(NULL,
                       cvUTF8ToWideString(
                         _("The game clients are out of sync. Would you like to send debug information to RttR to help us avoiding this in "
                           "the future? Thank you very much!"))
                         .c_str(),
                       cvUTF8ToWideString(_("Error")).c_str(), MB_YESNO | MB_ICONERROR | MB_TASKMODAL | MB_SETFOREGROUND)
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
    int newColorIdx = player.GetColorIdx(newColor);
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
    if(status != SS_GAME && status != SS_CONFIG)
        return true;
    int targetPlayer = GetTargetPlayer(msg);
    if(targetPlayer < 0)
        return true;
    uint8_t player1 = static_cast<uint8_t>(targetPlayer);
    if(player1 == msg.player2 || msg.player2 >= playerInfos.size())
        return true;

    SwapPlayer(player1, msg.player2);
    return true;
}

void GameServer::SwapPlayer(const uint8_t player1, const uint8_t player2)
{
    // TODO: Swapping the player messes up the ids because our IDs are indizes. Usually this works because we use the sender player ID for
    // messages received by the server and set the right ID for messages sent by the server. However (currently only) the host may send
    // messages for another player. Those will not get the adjusted ID till he gets the swap message. So there is a short time, where the
    // messages may be executed for the wrong player.
    // Idea: Use actual IDs for players in messages (unique)
    if(status == SS_CONFIG)
    {
        // Swap player during match-making
        // Swap everything
        using std::swap;
        swap(playerInfos[player1], playerInfos[player2]);
        // In savegames some things cannot be changed
        if(mapinfo.type == MAPTYPE_SAVEGAME)
            playerInfos[player1].FixSwappedSaveSlot(playerInfos[player2]);
    } else if(status == SS_GAME)
    {
        // Ingame we can only switch to a KI
        if(playerInfos[player1].ps != PS_OCCUPIED || playerInfos[player2].ps != PS_AI)
            return;

        LOG.write("GameServer::ChangePlayer %i - %i \n") % unsigned(player1) % unsigned(player2);
        using std::swap;
        swap(playerInfos[player2].ps, playerInfos[player1].ps);

        // Alte KI löschen
        delete ai_players[player2];
        ai_players[player2] = NULL;
        // Place a dummy AI at the original spot
        ai_players[player1] = GAMECLIENT.CreateAIPlayer(player1, AI::Info(AI::DUMMY));
    }
    // Change ids of network players (if any). Get both first!
    GameServerPlayer* newPlayer = GetNetworkPlayer(player2);
    GameServerPlayer* oldPlayer = GetNetworkPlayer(player1);
    if(newPlayer)
        newPlayer->playerId = player1;
    if(oldPlayer)
        oldPlayer->playerId = player2;
    SendToAll(GameMessage_Player_Swap(player1, player2));
}

GameServerPlayer* GameServer::GetNetworkPlayer(unsigned playerId)
{
    BOOST_FOREACH(GameServerPlayer& player, networkPlayers)
    {
        if(player.playerId == playerId)
            return &player;
    }
    return NULL;
}

void GameServer::SetPaused(bool paused)
{
    if(framesinfo.isPaused == paused)
        return;
    framesinfo.isPaused = paused;
    SendToAll(GameMessage_Pause(framesinfo.isPaused));
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
            return msg.player;
    } else if(msg.senderPlayerID < playerInfos.size())
        return msg.senderPlayerID;
    return -1;
}
