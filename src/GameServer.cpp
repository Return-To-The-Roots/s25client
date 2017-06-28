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

#include "defines.h" // IWYU pragma: keep
#include <build_version.h>
#include "GameServer.h"

#include "SocketSet.h"

#include "Loader.h"
#include "drivers/VideoDriverWrapper.h"
#include "GameMessage.h"
#include "GameMessages.h"
#include "GameClient.h"

#include "GlobalGameSettings.h"
#include "LobbyClient.h"
#include "ingameWindows/iwDirectIPCreate.h"

#include "GameServerPlayer.h"
#include "GameManager.h"
#include "Savegame.h"
#include "ai/AIBase.h"
#include "Settings.h"
#include "Debug.h"
#include "fileFuncs.h"
#include "ogl/glArchivItem_Map.h"
#include "ogl/glArchivItem_Bitmap.h"
#include "GameMessage_GameCommand.h"
#include "gameData/GameConsts.h"
#include "gameData/LanDiscoveryCfg.h"
#include "gameTypes/LanGameInfo.h"

#include "helpers/Deleter.h"
#include "libsiedler2/src/prototypen.h"
#include "libsiedler2/src/ArchivItem_Map_Header.h"
#include "libutil/src/colors.h"
#include "libutil/src/ucString.h"

#include "files.h"
#include <boost/foreach.hpp>
#include <boost/filesystem.hpp>
#include <fstream>

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

GameServer::CountDown::CountDown(): isActive(false), remainingSecs(0), lasttime(0)
{}

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
GameServer::GameServer(): lanAnnouncer(LAN_DISCOVERY_CFG)
{
    status = SS_STOPPED;

    async_player1 = async_player2 = -1;
    async_player1_done = async_player2_done = false;
    framesinfo.Clear();
    config.Clear();
    mapinfo.Clear();
    skiptogf=0;
}

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
        default: LOG.write("GameServer::Start: ERROR: Map-Type %u not supported!\n") % mapinfo.type; return false;
        // Altes S2-Mapformat von BB
        case MAPTYPE_OLDMAP:
        {
            libsiedler2::ArchivInfo map;

            // Karteninformationen laden
            if(libsiedler2::loader::LoadMAP(mapinfo.filepath, map, true) != 0)
            {
                LOG.write("GameServer::Start: ERROR: Map \"%s\", couldn't load header!\n") % mapinfo.filepath;
                return false;
            }
            const libsiedler2::ArchivItem_Map_Header* header = &(dynamic_cast<const glArchivItem_Map*>(map.get(0))->getHeader());
            RTTR_Assert(header);

            config.playercount = header->getPlayer();
            mapinfo.title = cvStringToUTF8(header->getName());
        } break;
        // Gespeichertes Spiel
        case MAPTYPE_SAVEGAME:
        {
            Savegame save;

            if(!save.Load(mapinfo.filepath, false, false))
                return false;

            // Spieleranzahl
            config.playercount = save.GetPlayerCount();
            bfs::path mapPath = map_path;
            mapinfo.title = save.mapName;
        } break;
    }

    status = SS_CREATING_LOBBY;
    // Von Lobby abhängig? Dann der Bescheid sagen und auf eine Antwort warten, dass wir den Server
    // erstellen dürfen
    if(config.servertype == ServerType::LOBBY)
    {
        LOBBYCLIENT.AddServer(config.gamename, mapinfo.title, (config.password.length() != 0), config.port);
        return true;
    }
    else
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

    // Speicher für Spieler anlegen
    players.clear();
    players.resize(config.playercount);

    // Potentielle KI-Player anlegen
    ai_players.resize(config.playercount);

    bool host_found = false;

    //// Spieler 0 erstmal der Host
    switch(mapinfo.type)
    {
        default:
            break;

        case MAPTYPE_OLDMAP:
        {
            // Host bei normalen Spieler der erste Spieler
            players[0].isHost = true;

            ggs_.LoadSettings();
        } break;
        case MAPTYPE_SAVEGAME:
        {
            Savegame save;
            if(!save.Load(mapinfo.filepath, true, false))
                return false;

            // Bei Savegames die Originalspieldaten noch mit auslesen
            for(unsigned i = 0; i < players.size(); ++i)
            {
                // PlayerState
                const BasePlayerInfo& savePlayer = save.GetPlayer(i);
                players[i].ps = savePlayer.ps;

                if(players[i].isUsed())
                {
                    players[i].aiInfo = savePlayer.aiInfo;
                    // (ehemaliger) Spielername
                    players[i].originName = savePlayer.name;

                    // Volk, Team und Farbe
                    players[i].nation = savePlayer.nation;
                    players[i].color = savePlayer.color;
                    players[i].team = savePlayer.team;
                }

                if(players[i].ps == PS_OCCUPIED)
                {
                    // Besetzt --> freigeben, damit auch jemand reinkann
                    players[i].ps = PS_FREE;
                    // Erster richtiger Spieler? Dann ist das der Host später
                    if(!host_found)
                    {
                        players[i].isHost = true;
                        host_found = true;
                    }
                }else if(players[i].ps == PS_AI)
                    players[i].name = savePlayer.name;
                players[i].InitRating();
            }

            // Einstellungen aus dem Savegame für die Addons werden in Load geladen

            // Und die GGS
            ggs_ = save.ggs;
        } break;
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

// clear async logs if necessary

    async_player1_log.clear();
    async_player2_log.clear();

    if (config.servertype == ServerType::LAN)
        lanAnnouncer.Start();
    AnnounceStatusChange();

    return true;
}

unsigned GameServer::GetFilledSlots() const
{
    unsigned numFilled = 0;
    BOOST_FOREACH(const GameServerPlayer& player, players)
    {
        if (player.ps != PS_FREE)
            ++numFilled;
    }
    return numFilled;
}

void GameServer::AnnounceStatusChange()
{
    if (config.servertype == ServerType::LAN)
    {
        LanGameInfo info;
        info.name = config.gamename;
        info.hasPwd = !config.password.empty();
        info.map = mapinfo.title;
        info.curPlayer = GetFilledSlots();
        info.maxPlayer = players.size();
        info.port = config.port;
        info.isIPv6 = config.ipv6;
        info.version = GetWindowVersion();
        Serializer ser;
        info.Serialize(ser);
        lanAnnouncer.SetPayload(ser.GetData(), ser.GetLength());
    }
    else if (config.servertype == ServerType::LOBBY)
    {
        LOBBYCLIENT.UpdateServerPlayerCount(GetFilledSlots(), players.size());
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
                if (!StartGame())
                {
                    GAMEMANAGER.ShowMenu();
                    return;
                }
            }
            else
            {
                SendToAll(GameMessage_Server_Countdown(countdown.GetRemainingSecs()));
                LOG.writeToFile("SERVER >>> BROADCAST: NMS_SERVER_COUNTDOWN(%d)\n") % countdown.GetRemainingSecs();
            }
        }
    }

    // queues abarbeiten
    for(unsigned id = 0; id < players.size(); ++id)
    {
        GameServerPlayer& player = players[id];

        // maximal 10 Pakete verschicken
        player.send_queue.send(player.so, 10);

        // recv-queue abarbeiten
        while(player.recv_queue.count() > 0)
        {
            player.recv_queue.front()->run(this, id);
            player.recv_queue.pop();
        }
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
    players.clear();

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
    skiptogf=0;

    lanAnnouncer.Stop();

    if(LOBBYCLIENT.IsLoggedIn()) // steht die Lobbyverbindung noch?
        LOBBYCLIENT.DeleteServer();

    // status
    status = SS_STOPPED;
    LOG.write("server state changed to stop\n");
}

/**
 *  startet den Spielstart-Countdown
 */
bool GameServer::StartCountdown()
{
    int playerCount = 0;

    // Alle Spieler da?
    BOOST_FOREACH(const GameServerPlayer& player, players)
    {
        // noch nicht alle spieler da -> feierabend!
        if( (player.ps == PS_FREE) || (player.ps == PS_RESERVED) )
            return false;
        else if(player.isHuman() && !player.isReady)
            return false;
        if(player.isHuman())
            playerCount++;
    }

    std::set<unsigned> takenColors;

    // Check all players have different colors
    BOOST_FOREACH(const GameServerPlayer& player, players)
    {
        if(player.isUsed())
        {
            if(helpers::contains(takenColors, player.color))
                return false;
            takenColors.insert(player.color);
        }
    }

    // Start countdown (except its single player)
    if(playerCount > 1)
    {
        countdown.Start(3, VIDEODRIVER.GetTickCount());
        SendToAll(GameMessage_Server_Countdown(countdown.GetRemainingSecs()));
        LOG.writeToFile("SERVER >>> Countdown started(%d)\n") % countdown.GetRemainingSecs();
    }else if(!StartGame())
    {
        GAMEMANAGER.ShowMenu();
        // Countdown was started (->true). Gamestart failed...
        return true;
    }

    return true;
}

/**
 *  stoppt den Spielstart-Countdown
 */
void GameServer::CancelCountdown()
{
    // Countdown-Stop allen mitteilen
    countdown.Stop();
    SendToAll(GameMessage_Server_CancelCountdown());
    LOG.writeToFile("SERVER >>> BROADCAST: NMS_SERVER_CANCELCOUNTDOWN\n");
}

/**
 *  startet das Spiel.
 */
bool GameServer::StartGame()
{
    lanAnnouncer.Stop();

    // Bei Savegames wird der Startwert von den Clients aus der Datei gelesen!
    unsigned random_init = (mapinfo.type == MAPTYPE_SAVEGAME) ? 0xFFFFFFFF : VIDEODRIVER.GetTickCount();

    // Höchsten Ping ermitteln
    unsigned highest_ping = 0;
    BOOST_FOREACH(const GameServerPlayer& player, players)
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
    for( ; i < 20; ++i)
    {
        if(i * framesinfo.gf_length > highest_ping + 200)
            break;
    }

    framesinfo.nwf_length = i;

    GameMessage_Server_Start start_msg(random_init, framesinfo.nwf_length);

    LOG.write("SERVER: Using gameframe length of %dms\n") % framesinfo.gf_length;
    LOG.write("SERVER: Using networkframe length of %u GFs (%ums)\n") % framesinfo.nwf_length % (framesinfo.nwf_length * framesinfo.gf_length);

    // Spielstart allen mitteilen
    SendToAll(start_msg);
    LOG.writeToFile("SERVER >>> BROADCAST: NMS_SERVER_START(%d)\n") % random_init;

    framesinfo.lastTime = VIDEODRIVER.GetTickCount();

    try
    {
        // GameClient soll erstmal starten, damit wir von ihm die benötigten Daten für die KIs bekommen
        GAMECLIENT.StartGame(random_init);
    }
    catch (SerializedGameData::Error& error)
    {
        LOG.write("Error when loading game: %s\n") % error.what();
        return false;
    }

    // Init current GF
    currentGF = GAMECLIENT.GetGFNumber();

    // Erste KI-Nachrichten schicken
    for(unsigned i = 0; i < players.size(); ++i)
    {
        if(players[i].ps == PS_AI)
        {
            SendNothingNC(i);
            ai_players[i] = GAMECLIENT.CreateAIPlayer(i, players[i].aiInfo);
        }
    }

    LOG.writeToFile("SERVER >>> BROADCAST: NMS_NWF_DONE\n");

    // Spielstart allen mitteilen
    SendToAll(GameMessage_Server_NWFDone(0xff, currentGF, framesinfo.gf_length, true));

    // ab ins game wechseln
    status = SS_GAME;

    return true;
}

///////////////////////////////////////////////////////////////////////////////
// wechselt Spielerstatus durch
void GameServer::TogglePlayerState(unsigned char playerId)
{
    GameServerPlayer& player = players[playerId];

    // oh ein spieler, weg mit ihm!
    if(player.isHuman())
    {
        KickPlayer(playerId, NP_NOCAUSE, 0);
        return;
    }

    // playerstatus weiterwechseln
    switch(player.ps)
    {
        default: break;
        case PS_FREE:
        {
            player.ps = PS_AI;
            player.aiInfo = AI::Info(AI::DEFAULT, AI::EASY);
        } break;
        case PS_AI:
            {
                // Verschiedene KIs durchgehen
                switch(player.aiInfo.type)
                {
                case AI::DEFAULT:
                    switch(player.aiInfo.level)
                    {
                    case AI::EASY:
                        player.aiInfo.level = AI::MEDIUM;
                        break;
                    case AI::MEDIUM:
                        player.aiInfo.level = AI::HARD;
                        break;
                    case AI::HARD:
                        player.aiInfo = AI::Info(AI::DUMMY);
                        break;
                    }
                    break;
                case AI::DUMMY:
                    if(mapinfo.type != MAPTYPE_SAVEGAME)
                        player.ps = PS_LOCKED;
                    else
                        player.ps = PS_FREE;
                    break;
                default:
                    if(mapinfo.type != MAPTYPE_SAVEGAME)
                        player.ps = PS_LOCKED;
                    else
                        player.ps = PS_FREE;
                    break;
                }
                break;
            }


        case PS_LOCKED:
        {
            // Im Savegame können auf geschlossene Slots keine Spieler
            // gesetzt werden, der entsprechende Spieler existierte ja gar nicht auf
            // der Karte!
            if(mapinfo.type != MAPTYPE_SAVEGAME)
                player.ps = PS_FREE;
        } break;
    }
    if(player.ps == PS_AI)
        player.SetAIName(playerId);
    player.isReady = (player.ps == PS_AI);

    // Tat verkünden
    SendToAll(GameMessage_Player_Set_State(playerId, player.ps, player.aiInfo));
    AnnounceStatusChange();

    // If slot is filled, check current color
    if(player.isUsed())
        CheckAndSetColor(playerId, player.color);
}

///////////////////////////////////////////////////////////////////////////////
// Team der KI ändern
void GameServer::ToggleAITeam(unsigned char playerId)
{
    GameServerPlayer& player = players[playerId];

    // nur KI
    if(player.ps != PS_AI)
        return;

    // team wechseln
    Team newTeam;
    //switch from random team?
    if(player.team == TM_RANDOMTEAM || player.team == TM_RANDOMTEAM2 || player.team == TM_RANDOMTEAM3 || player.team == TM_RANDOMTEAM4)
        newTeam = TM_TEAM1;
    else if(player.team == TM_NOTEAM) //switch to random team?
    {
        int randomTeamNum = rand() % 4;
        if(randomTeamNum == 0)
            newTeam = TM_RANDOMTEAM;
        else
            newTeam = Team(TM_RANDOMTEAM2 + randomTeamNum - 1);
    } else
        newTeam = Team((player.team + 1) % TEAM_COUNT);
    OnGameMessage(GameMessage_Player_Set_Team(playerId, newTeam));
}

///////////////////////////////////////////////////////////////////////////////
// Farbe der KI ändern
void GameServer::ToggleAIColor(unsigned char playerId)
{
    GameServerPlayer& player = players[playerId];

    // nur KI
    if(player.ps != PS_AI)
        return;

    CheckAndSetColor(playerId, PLAYER_COLORS[(player.GetColorIdx() + 1) % PLAYER_COLORS.size()]);
}

///////////////////////////////////////////////////////////////////////////////
// Nation der KI ändern
void GameServer::ToggleAINation(unsigned char playerId)
{
    GameServerPlayer& player = players[playerId];

    // nur KI
    if(player.ps != PS_AI)
        return;

    // Nation wechseln
    OnGameMessage(GameMessage_Player_Set_Nation(playerId, Nation((player.nation + 1) % NAT_COUNT)));
}

///////////////////////////////////////////////////////////////////////////////
// Spieleinstellungen verschicken
void GameServer::ChangeGlobalGameSettings(const GlobalGameSettings& ggs)
{
    this->ggs_ = ggs;
    SendToAll(GameMessage_GGSChange(ggs));
    LOG.writeToFile("SERVER >>> BROADCAST: NMS_GGS_CHANGE\n");
}

void GameServer::RemoveLuaScript()
{
    RTTR_Assert(status == SS_CONFIG);
    mapinfo.luaFilepath.clear();
    mapinfo.luaData.Clear();
    mapinfo.luaChecksum = 0;
    SendToAll(GameMessage_RemoveLua());
}

/**
 *  Nachricht an Alle
 */
void GameServer::SendToAll(const GameMessage& msg)
{
    for(std::vector<GameServerPlayer>::iterator it = players.begin(); it != players.end(); ++it)
    {
        // ist der Slot Belegt, dann Nachricht senden
        if(it->ps == PS_OCCUPIED)
            it->send_queue.push(msg.duplicate());
    }
}

///////////////////////////////////////////////////////////////////////////////
// kickt einen spieler und räumt auf
void GameServer::KickPlayer(unsigned char playerId, unsigned char cause, unsigned short param)
{
    GameServerPlayer& player = players[playerId];
    PlayerState oldPs = player.ps;

    // send-queue flushen
    player.send_queue.flush(player.so);
    player.CloseConnections();

    // If we are ingame, replace by KI
    if(status == SS_GAME)
    {
        // KI-Spieler muss übernehmen
        player.ps = PS_AI;
        player.aiInfo.type = AI::DUMMY;
        player.aiInfo.level = AI::MEDIUM;
        ai_players[playerId] = GAMECLIENT.CreateAIPlayer(playerId, player.aiInfo);
    }

    // Do not send notifications if the player was not already there
    if(oldPs == PS_RESERVED)
        return;

    SendToAll(GameMessage_Player_Kicked(playerId, cause, param));
    AnnounceStatusChange();
    LOG.writeToFile("SERVER >>> BROADCAST: NMS_PLAYERKICKED(%d,%d,%d)\n") % playerId % cause % param;

    if(status == SS_GAME)
        SendNothingNC(playerId);
}

void GameServer::KickPlayer(unsigned playerIdx)
{
    KickPlayer(playerIdx, NP_NOCAUSE, 0);
}

///////////////////////////////////////////////////////////////////////////////
// testet, ob in der Verbindungswarteschlange Clients auf Verbindung warten
void GameServer::ClientWatchDog()
{
    SocketSet set;
    set.Clear();

    // sockets zum set hinzufügen
    BOOST_FOREACH(GameServerPlayer& player, players)
    {
        if(player.isHuman())
            set.Add(player.so);
    }

    // auf fehler prüfen
    if(set.Select(0, 2) > 0)
    {
        for(unsigned id = 0; id < players.size(); ++id)
        {
            if(set.InSet(players[id].so))
            {
                LOG.write("SERVER: Error on socket of player %d, bye bye!\n") % id;
                KickPlayer(id, NP_CONNECTIONLOST, 0);
            }
        }
    }

    BOOST_FOREACH(GameServerPlayer& player, players)
    {
        player.doPing();
        player.checkConnectTimeout();
    }
}

void GameServer::ExecuteGameFrame()
{
    RTTR_Assert(status == SS_GAME);

    if(framesinfo.isPaused)
        return;

    unsigned int currentTime = VIDEODRIVER.GetTickCount();

    // prüfen ob GF vergangen
    if(currentTime - framesinfo.lastTime >= framesinfo.gf_length || skiptogf > currentGF)
    {
        // NWF vergangen?
        if(currentGF % framesinfo.nwf_length==0)
        {
            const unsigned char laggingPlayerIdx = GetLaggingPlayer();
            if(laggingPlayerIdx != 0xFF)
                CheckAndKickLaggingPlayer(laggingPlayerIdx);
            else
            {
                ExecuteNWF(currentTime);
                // Execute RunGF AFTER the NWF is send.
                RunGF(true);
            }
        }
        else
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
    for(unsigned i = 0; i < ai_players.size(); ++i)
    {
        if(ai_players[i])
            ai_players[i]->RunGF(currentGF, isNWF);
    }
    ++currentGF;
}

void GameServer::ExecuteNWF(const unsigned  /*currentTime*/)
{
    // Advance lastExecutedTime by the GF length.
    // This is not really the last executed time, but if we waited (or laggt) we can catch up a bit by executing the next GF earlier
    framesinfo.lastTime += framesinfo.gf_length;

    // We take the checksum of the first human player as the reference
    unsigned char referencePlayerIdx = 0xFF;
    AsyncChecksum referenceChecksum;
    std::vector<int> checksums;
    checksums.reserve(players.size());

    // Send AI commands and check for asyncs
    for(unsigned playerId = 0; playerId < players.size(); ++playerId)
    {
        GameServerPlayer& player = players[playerId];

        // Befehle der KI senden
        if(player.ps == PS_AI)
        {
            //LOG.writeToFile("SERVER >>> GC %u\n") % playerId;
            SendToAll(GameMessage_GameCommand(playerId, AsyncChecksum(0), ai_players[playerId]->GetGameCommands()));
            ai_players[playerId]->FetchGameCommands();
            RTTR_Assert(player.gc_queue.empty());
            continue; // No GCs in the queue for KIs
        }

        if(player.ps != PS_OCCUPIED)
            continue; // No player

        RTTR_Assert(!player.gc_queue.empty()); // Players should not be lagging at this point

        // Spieler laggt nicht (mehr ggf)
        player.NotLagging();

        const GameMessage_GameCommand& frontGC = player.gc_queue.front();
        AsyncChecksum curChecksum = frontGC.checksum;
        checksums.push_back(curChecksum.randChecksum);

        // Checksumme des ersten Spielers als Richtwert
        if (referencePlayerIdx == 0xFF)
        {
            referenceChecksum = curChecksum;
            referencePlayerIdx = playerId;
        }

        // Remove first (current) msg
        player.gc_queue.erase(player.gc_queue.begin());
        RTTR_Assert(player.gc_queue.size() <= 1); // At most 1 additional GC-Message, otherwise the client skipped a NWF

        // Checksummen nicht gleich?
        if (curChecksum != referenceChecksum)
        {
            LOG.write("Async at GF %u of players %u vs %u: Checksum %i:%i ObjCt %u:%u ObjIdCt %u:%u\n") % currentGF % playerId % referencePlayerIdx
                % curChecksum.randChecksum % referenceChecksum.randChecksum
                % curChecksum.objCt % referenceChecksum.objCt
                % curChecksum.objIdCt % referenceChecksum.objIdCt;

            // AsyncLog der asynchronen Player anfordern
            if (async_player1 == -1)
            {
                GameServerPlayer& refPlayer = players[referencePlayerIdx];
                async_player1 = referencePlayerIdx;
                async_player1_done = false;
                refPlayer.send_queue.push(new GameMessage_GetAsyncLog(async_player1));
                refPlayer.send_queue.flush(refPlayer.so);

                async_player2 = playerId;
                async_player2_done = false;
                player.send_queue.push(new GameMessage_GetAsyncLog(playerId));
                player.send_queue.flush(player.so);

                // Async-Meldung rausgeben.
                SendToAll(GameMessage_Server_Async(checksums));

                // Spiel pausieren
                RTTR_Assert(!framesinfo.isPaused);
                SetPaused(true);
            }
        }
    }

    if(framesinfo.gfLenghtNew != framesinfo.gf_length)
    {
        unsigned oldGfLen = framesinfo.gf_length;
        unsigned oldnNwfLen = framesinfo.nwf_length;
        framesinfo.ApplyNewGFLength();

        LOG.write("Server %d/%d: Speed changed from %d to %d. NWF %u to %u\n") % currentGF % currentGF % oldGfLen % framesinfo.gf_length % oldnNwfLen % framesinfo.nwf_length;
    }

    framesinfo.gfLenghtNew = framesinfo.gfLenghtNew2;

    SendToAll(GameMessage_Server_NWFDone(0xff, currentGF, framesinfo.gfLenghtNew));
}

void GameServer::CheckAndKickLaggingPlayer(const unsigned char playerIdx)
{
    // ein spieler laggt, spiel pausieren
    GameServerPlayer& player = players[playerIdx];
    player.Lagging();
    const unsigned timeOut = player.GetTimeOut();
    if(timeOut == 0)
        KickPlayer(playerIdx, NP_PINGTIMEOUT, 0);
    else if(timeOut <= 30 && (timeOut % 5 == 0 || timeOut < 5)) // Notify every 5s if max 30s are remaining, if less than 5s notify every second
        LOG.write("SERVER: Kicke Spieler %d in %u Sekunden\n") % playerIdx % timeOut;
}

unsigned char GameServer::GetLaggingPlayer() const
{
    for(unsigned char playerId = 0; playerId < players.size(); ++playerId)
    {
        const GameServerPlayer& player = players[playerId];

        if(player.ps == PS_OCCUPIED && player.gc_queue.empty())
        {
            // der erste, der erwischt wird, bekommt den zuschlag ;-)
            return playerId;
        }
    }
    return 0xFF;
}

/**
 *  Sendet ein NC-Paket ohne Befehle.
 */
void GameServer::SendNothingNC(const unsigned int& id)
{
    SendToAll(GameMessage_GameCommand(id, AsyncChecksum(0), std::vector<gc::GameCommandPtr>()));
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

            unsigned char newPlayerId = 0xFF;
            // Geeigneten Platz suchen
            for(unsigned playerId = 0; playerId < players.size(); ++playerId)
            {
                if(players[playerId].ps == PS_FREE)
                {
                    // platz reservieren
                    players[playerId].reserve(socket);
                    newPlayerId = playerId;
                    //LOG.write(("new socket, about to tell him about his playerId: %i \n",playerId);
                    // schleife beenden
                    break;
                }
            }

            GameMessage_Player_Id msg(newPlayerId);
            MessageHandler::send(socket, msg);

            // war kein platz mehr frei, wenn ja dann verbindung trennen?
            if(newPlayerId == 0xFF)
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
        BOOST_FOREACH(GameServerPlayer& player, players)
        {
            if(player.isHuman())
                set.Add(player.so);
        }

        msgReceived = false;

        // ist eines der Sockets im Set lesbar?
        if(set.Select(0, 0) > 0)
        {
            for(unsigned id = 0; id < players.size(); ++id)
            {
                if( players[id].isHuman() && set.InSet(players[id].so) )
                {
                    // nachricht empfangen
                    if(!players[id].recv_queue.recv(players[id].so))
                    {
                        LOG.write("SERVER: Receiving Message for player %d failed, kick it like Beckham!\n") % id;
                        KickPlayer(id, NP_CONNECTIONLOST, 0);
                    } else
                        msgReceived = true;
                }
            }
        }
    }
    while(msgReceived);
}

///////////////////////////////////////////////////////////////////////////////
// pongnachricht
inline void GameServer::OnGameMessage(const GameMessage_Pong& msg)
{
    if(msg.player >= GetMaxPlayerCount())
        return;

    GameServerPlayer& player = players[msg.player];

    unsigned int currenttime = VIDEODRIVER.GetTickCount();

    player.ping = (unsigned short)(currenttime - player.lastping);
    player.pinging = false;
    player.lastping = currenttime;

    // Den neuen Ping allen anderen Spielern Bescheid sagen
    GameMessage_Player_Ping ping_msg(msg.player, player.ping);
    SendToAll(ping_msg);
}

///////////////////////////////////////////////////////////////////////////////
// servertype
inline void GameServer::OnGameMessage(const GameMessage_Server_Type& msg)
{
    if(msg.player >= GetMaxPlayerCount())
        return;

    GameServerPlayer& player = players[msg.player];

    int typok = 0;
    if(msg.type != config.servertype)
        typok = 1;
    else if(msg.version != GetWindowVersion())
        typok = 2;

    player.send_queue.push(new GameMessage_Server_TypeOK(typok));

    if(typok != 0)
        KickPlayer(msg.player, NP_CONNECTIONLOST, 0);
}

/**
 *  Server-Passwort-Nachricht
 */
void GameServer::OnGameMessage(const GameMessage_Server_Password& msg)
{
    if(msg.player >= GetMaxPlayerCount())
        return;

    GameServerPlayer& player = players[msg.player];

    std::string passwordok = (config.password == msg.password ? "true" : "false");

    player.send_queue.push(new GameMessage_Server_Password(passwordok));

    if(passwordok == "false")
        KickPlayer(msg.player, NP_WRONGPASSWORD, 0);
}

/**
 *  Server-Chat-Nachricht.
 */
void GameServer::OnGameMessage(const GameMessage_Server_Chat& msg)
{
    SendToAll(msg);
}

void GameServer::OnGameMessage(const GameMessage_System_Chat& msg)
{
    SendToAll(msg);
}

///////////////////////////////////////////////////////////////////////////////
// Spielername
inline void GameServer::OnGameMessage(const GameMessage_Player_Name& msg)
{
    if(msg.player >= GetMaxPlayerCount())
        return;

    GameServerPlayer& player = players[msg.player];

    LOG.writeToFile("CLIENT%d >>> SERVER: NMS_PLAYER_NAME(%s)\n") % msg.player % msg.playername;

    player.name = msg.playername;

    // Als Antwort Karteninformationen übertragen
    player.send_queue.push(new GameMessage_Map_Info(bfs::path(mapinfo.filepath).filename().string(), mapinfo.type,
        mapinfo.mapData.length, mapinfo.mapData.data.size(),
        mapinfo.luaData.length, mapinfo.luaData.data.size()
        ));

    // Send map data
    unsigned curPos = 0;
    const unsigned compressedMapSize = mapinfo.mapData.data.size();
    while(curPos < compressedMapSize)
    {
        unsigned chunkSize = (compressedMapSize - curPos > MAP_PART_SIZE) ? MAP_PART_SIZE : (compressedMapSize - curPos);

        player.send_queue.push(new GameMessage_Map_Data(true, curPos, &mapinfo.mapData.data[curPos], chunkSize));
        curPos += chunkSize;
    }

    RTTR_Assert(curPos == compressedMapSize);

    // And lua data (if there is any)
    RTTR_Assert(mapinfo.luaFilepath.empty() == mapinfo.luaData.data.empty());
    RTTR_Assert(mapinfo.luaData.data.empty() == (mapinfo.luaData.length == 0));
    curPos = 0;
    const unsigned compressedLuaSize = mapinfo.luaData.data.size();
    while(curPos < compressedLuaSize)
    {
        unsigned chunkSize = (compressedLuaSize - curPos > MAP_PART_SIZE) ? MAP_PART_SIZE : (compressedLuaSize - curPos);

        player.send_queue.push(new GameMessage_Map_Data(false, curPos, &mapinfo.luaData.data[curPos], chunkSize));
        curPos += chunkSize;
    }

    RTTR_Assert(curPos == compressedLuaSize);
}


///////////////////////////////////////////////////////////////////////////////
// Nation weiterwechseln
inline void GameServer::OnGameMessage(const GameMessage_Player_Set_Nation& msg)
{
    if(msg.player >= GetMaxPlayerCount())
        return;

    GameServerPlayer& player = players[msg.player];

    player.nation = msg.nation;

    LOG.writeToFile("CLIENT%d >>> SERVER: NMS_PLAYER_TOGGLENATION\n") % msg.player;

    // Nation-Change senden
    SendToAll(GameMessage_Player_Set_Nation(msg.player, msg.nation));

    LOG.writeToFile("SERVER >>> BROADCAST: NMS_PLAYER_TOGGLENATION(%d, %d)\n") % msg.player % player.nation;
}

///////////////////////////////////////////////////////////////////////////////
// Team weiterwechseln
inline void GameServer::OnGameMessage(const GameMessage_Player_Set_Team& msg)
{
    if(msg.player >= GetMaxPlayerCount())
        return;

    GameServerPlayer& player = players[msg.player];

    player.team = msg.team;

    LOG.writeToFile("CLIENT%d >>> SERVER: NMS_PLAYER_TOGGLETEAM\n") % msg.player;
    SendToAll(GameMessage_Player_Set_Team(msg.player, msg.team));
    LOG.writeToFile("SERVER >>> BROADCAST: NMS_PLAYER_TOGGLETEAM(%d, %d)\n") % msg.player % player.team;
}

///////////////////////////////////////////////////////////////////////////////
// Farbe weiterwechseln
inline void GameServer::OnGameMessage(const GameMessage_Player_Set_Color& msg)
{
    if(msg.player >= GetMaxPlayerCount())
        return;

    LOG.writeToFile("CLIENT%u >>> SERVER: NMS_PLAYER_TOGGLECOLOR %u\n") % msg.player % msg.color;
    CheckAndSetColor(msg.player, msg.color);
}

/**
 *  Spielerstatus wechseln
 */
inline void GameServer::OnGameMessage(const GameMessage_Player_Ready& msg)
{
    if(msg.player >= GetMaxPlayerCount())
        return;

    GameServerPlayer& player = players[msg.player];

    player.isReady = msg.ready;

    // countdown ggf abbrechen
    if(!player.isReady && countdown.IsActive())
        CancelCountdown();

    LOG.writeToFile("CLIENT%d >>> SERVER: NMS_PLAYER_READY(%s)\n") % msg.player % (player.isReady ? "true" : "false");
    // Broadcast to all players
    SendToAll(msg);
    LOG.writeToFile("SERVER >>> BROADCAST: NMS_PLAYER_READY(%d, %s)\n") % msg.player % (player.isReady ? "true" : "false");
}

///////////////////////////////////////////////////////////////////////////////
// Checksumme
inline void GameServer::OnGameMessage(const GameMessage_Map_Checksum& msg)
{
    if(msg.player >= GetMaxPlayerCount())
        return;

    GameServerPlayer& player = players[msg.player];

    bool checksumok = (msg.mapChecksum == mapinfo.mapChecksum && msg.luaChecksum == mapinfo.luaChecksum);

    LOG.writeToFile("CLIENT%d >>> SERVER: NMS_MAP_CHECKSUM(%u) expected: %u, ok: %s\n") % msg.player % msg.mapChecksum % mapinfo.mapChecksum % (checksumok ? "yes" : "no");

    // Antwort senden
    player.send_queue.push(new GameMessage_Map_ChecksumOK(checksumok));

    LOG.writeToFile("SERVER >>> CLIENT%d: NMS_MAP_CHECKSUM(%d)\n") % msg.player % checksumok;

    if(!checksumok)
        KickPlayer(msg.player, NP_WRONGCHECKSUM, 0);
    else
    {
        // den anderen Spielern mitteilen das wir einen neuen haben
        SendToAll(GameMessage_Player_New(msg.player, player.name));

        LOG.writeToFile("SERVER >>> BROADCAST: NMS_PLAYER_NEW(%d, %s)\n") % msg.player % player.name;

        // belegt markieren
        player.ps = PS_OCCUPIED;
        player.lastping = VIDEODRIVER.GetTickCount();

        // Servername senden
        player.send_queue.push(new GameMessage_Server_Name(config.gamename));

        // Spielerliste senden
        player.send_queue.push(new GameMessage_Player_List(std::vector<JoinPlayerInfo>(players.begin(), players.end())));

        // Assign unique color
        CheckAndSetColor(msg.player, player.color);

        // GGS senden
        player.send_queue.push(new GameMessage_GGSChange(ggs_));

        AnnounceStatusChange();

        LOG.writeToFile("SERVER >>> BROADCAST: NMS_GGS_CHANGE\n");
    }
}

// speed change message
void GameServer::OnGameMessage(const GameMessage_Server_Speed& msg)
{
    framesinfo.gfLenghtNew2 = msg.gf_length;
}

void GameServer::OnGameMessage(const GameMessage_GameCommand& msg)
{
    if(msg.player >= GetMaxPlayerCount())
        return;

    GameServerPlayer& player = players[msg.player];
    //LOG.writeToFile("SERVER <<< GC %u\n") % msg.player;

    // Only valid from humans (for now)
    if(player.ps != PS_OCCUPIED)
        return;

    // Save and broadcast command
    player.gc_queue.push_back(msg);
    SendToAll(msg);
}

void GameServer::OnGameMessage(const GameMessage_SendAsyncLog& msg)
{
    if (msg.player == async_player1)
    {
        async_player1_log.insert(async_player1_log.end(), msg.entries.begin(), msg.entries.end());

        if (msg.last)
        {
            LOG.write("Received async logs from %u (%lu entries).\n") % async_player1 % async_player1_log.size();
            async_player1_done = true;
        }
    }
    else if (msg.player == async_player2)
    {
        async_player2_log.insert(async_player2_log.end(), msg.entries.begin(), msg.entries.end());

        if (msg.last)
        {
            LOG.write("Received async logs from %u (%lu entries).\n") % async_player2 % async_player2_log.size();
            async_player2_done = true;
        }
    }
    else
    {
        LOG.write("Received async log from %u, but did not expect it!\n") % msg.player;
        return;
    }

    // list is not yet complete, keep it coming...
    if (!async_player1_done || !async_player2_done)
        return;

    LOG.write("Async logs received completely.\n");

    std::vector<RandomEntry>::const_iterator it1 = async_player1_log.begin();
    std::vector<RandomEntry>::const_iterator it2 = async_player2_log.begin();

    // compare counters, adjust them so we're comparing the same counter numbers
    if (it1->counter > it2->counter)
    {
        for(; it2 != async_player2_log.end(); ++it2)
        {
            if (it2->counter == it1->counter)
                break;
        }
    }
    else if (it1->counter < it2->counter)
    {
        for(; it1 != async_player1_log.end(); ++it1)
        {
            if (it2->counter == it1->counter)
                break;
        }
    }

    // count identical lines
    unsigned identical = 0;
    while ((it1 != async_player1_log.end()) && (it2 != async_player2_log.end()) &&
            (it1->max == it2->max) && (it1->rngState == it2->rngState) && (it1->obj_id == it2->obj_id))
    {
        ++identical; ++it1; ++it2;
    }

    it1 -= identical;
    it2 -= identical;

    LOG.write("There are %u identical async log entries.\n") % identical;

    if (SETTINGS.global.submit_debug_data == 1
#ifdef _WIN32
            || (MessageBoxA(NULL,
                            _("The game clients are out of sync. Would you like to send debug information to RttR to help us avoiding this in the future? Thank you very much!"),
                            _("Error"), MB_YESNO | MB_ICONERROR | MB_TASKMODAL | MB_SETFOREGROUND) == IDYES)
#endif
       )
    {
        DebugInfo di;
        LOG.write("Sending async logs %s.\n") %
                    (di.SendAsyncLog(it1, it2, async_player1_log, async_player2_log, identical) ? "succeeded" : "failed");

        di.SendReplay();
    }

    std::string fileName = GetFilePath(FILE_PATHS[47]) + TIME.FormatTime("async_%Y-%m-%d_%H-%i-%s") + "Server.log";

    // open async log
    bfs::ofstream file(fileName);

    if (file)
    {
        // print identical lines, they help in tracing the bug
        for(unsigned i = 0; i < identical; i++)
        {
            file << "[I]: " << *it1 << "\n";
            ++it1; ++it2;
        }

        while ((it1 != async_player1_log.end()) && (it2 != async_player2_log.end()))
        {
            file << "[S]: " << *it1 << "\n";
            file << "[C]: " << *it2 << "\n";
            ++it1; ++it2;
        }

        LOG.write("Async log saved at \"%s\"\n") % fileName;
    }
    else
    {
        LOG.write("Failed to save async log at \"%s\"\n") % fileName;
    }

    async_player1_log.clear();
    async_player2_log.clear();

    KickPlayer(msg.player, NP_ASYNC, 0);
}

void GameServer::CheckAndSetColor(unsigned playerIdx, unsigned newColor)
{
    RTTR_Assert(playerIdx < players.size());
    RTTR_Assert(players.size() <= PLAYER_COLORS.size()); // Else we may not find a valid color!

    GameServerPlayer& player = players[playerIdx];
    RTTR_Assert(player.isUsed()); // Should only set colors for taken spots

    // Get colors used by other players
    std::set<unsigned> takenColors;
    for(unsigned p = 0; p < players.size(); ++p)
    {
        // Skip self
        if(p == playerIdx)
            continue;

        GameServerPlayer& otherPlayer = players[p];
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

    SendToAll(GameMessage_Player_Set_Color(playerIdx, player.color));
    LOG.writeToFile("SERVER >>> BROADCAST: NMS_PLAYER_TOGGLECOLOR(%d, %d)\n") % playerIdx % player.color;
}

void GameServer::OnGameMessage(const GameMessage_Player_Swap& msg)
{
    if(msg.player >= GetMaxPlayerCount() || msg.player2 >= GetMaxPlayerCount())
        return;

    if(status != SS_GAME)
        return;
    if(msg.player == msg.player2)
        return;
    // old_id muss richtiger Spieler, new_id KI sein, ansonsten geht das natürlich nicht
    if(players[msg.player].ps != PS_OCCUPIED || players[msg.player2].ps != PS_AI)
        return;
    SendToAll(msg);
    ChangePlayer(msg.player, msg.player2);
}

void GameServer::ChangePlayer(const unsigned char old_id, const unsigned char new_id)
{
    RTTR_Assert(status == SS_GAME); // Change player only ingame

    LOG.write("GameServer::ChangePlayer %i - %i \n") % old_id % new_id;
    using std::swap;
    swap(players[new_id].ps, players[old_id].ps);
    swap(players[new_id].so, players[old_id].so);
    swap(players[new_id].send_queue, players[old_id].send_queue);
    swap(players[new_id].recv_queue, players[old_id].recv_queue);

    // Alte KI löschen
    delete ai_players[new_id];
    ai_players[new_id] = NULL;
    // Place a dummy AI at the original spot
    ai_players[old_id] = GAMECLIENT.CreateAIPlayer(old_id, AI::Info(AI::DUMMY));

    //swap the gamecommand queue
    swap(players[old_id].gc_queue, players[new_id].gc_queue);
}

void GameServer::SetPaused(bool paused)
{
    if(framesinfo.isPaused == paused)
        return;
    framesinfo.isPaused = paused;
    SendToAll(GameMessage_Pause(framesinfo.isPaused));
}

void GameServer::SwapPlayer(const unsigned char player1, const unsigned char player2)
{
    RTTR_Assert(status == SS_CONFIG); // Swap player during match-making
    SendToAll(GameMessage_Player_Swap(player1, player2));
    // Swap everything
    using std::swap;
    swap(players[player1], players[player2]);
	// In savegames some things cannot be changed
	if(mapinfo.type == MAPTYPE_SAVEGAME)
		players[player1].FixSwappedSaveSlot(players[player2]);
}

JoinPlayerInfo& GameServer::GetJoinPlayer(unsigned playerIdx)
{
    return players.at(playerIdx);
}
