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

///////////////////////////////////////////////////////////////////////////////
// Header
#include "defines.h" // IWYU pragma: keep
#include <build_version.h>
#include "GameServer.h"

#include "SocketSet.h"

#include "Loader.h"
#include "Random.h"
#include "drivers/VideoDriverWrapper.h"
#include "GameMessage.h"
#include "GameMessages.h"
#include "GameClient.h"

#include "GlobalGameSettings.h"
#include "LobbyClient.h"
#include "ingameWindows/iwDirectIPCreate.h"

#include "GameServerPlayer.h"
#include "GameManager.h"
#include "GameSavegame.h"
#include "ai/AIBase.h"
#include "ai/AIEvents.h"
#include "Settings.h"
#include "Debug.h"
#include "fileFuncs.h"
#include "ogl/glArchivItem_Map.h"
#include "ogl/glArchivItem_Bitmap.h"

#include "gameData/GameConsts.h"
#include "gameData/LanDiscoveryCfg.h"
#include "gameTypes/LanGameInfo.h"

#include "helpers/Deleter.h"
#include "libsiedler2/src/prototypen.h"
#include "libsiedler2/src/ArchivItem_Map_Header.h"
#include "libutil/src/colors.h"
#include "libutil/src/ucString.h"

#include "files.h"
#include <boost/filesystem.hpp>
#include <fstream>

// Include last!
#include "DebugNew.h" // IWYU pragma: keep

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

GameServer::CountDown::CountDown()
{
    Clear();
}

void GameServer::CountDown::Clear(int time)
{
    do_countdown = false;
    countdown = time;
    lasttime = 0;
}

///////////////////////////////////////////////////////////////////////////////
//
GameServer::GameServer(): lanAnnouncer(LAN_DISCOVERY_CFG)
{
    status = SS_STOPPED;

    async_player1 = async_player2 = -1;
    async_player1_done = async_player2_done = false;
    framesinfo.Clear();
    serverconfig.Clear();
    mapinfo.Clear();
    countdown.Clear();
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
    serverconfig.gamename = csi.gamename;
    serverconfig.password = csi.password;
    serverconfig.servertype = csi.type;
    serverconfig.port = csi.port;
    serverconfig.ipv6 = csi.ipv6;
    serverconfig.use_upnp = csi.use_upnp;
    mapinfo.type = map_type;
    mapinfo.filepath = map_path;

    // Maps, Random-Maps, Savegames - Header laden und relevante Informationen rausschreiben (Map-Titel, Spieleranzahl)
    switch(mapinfo.type)
    {
        default: LOG.lprintf("GameServer::Start: ERROR: Map-Type %u not supported!\n", mapinfo.type); return false;
        // Altes S2-Mapformat von BB
        case MAPTYPE_OLDMAP:
        {
            libsiedler2::ArchivInfo map;

            // Karteninformationen laden
            if(libsiedler2::loader::LoadMAP(mapinfo.filepath, map, true) != 0)
            {
                LOG.lprintf("GameServer::Start: ERROR: Map \"%s\", couldn't load header!\n", mapinfo.filepath.c_str());
                return false;
            }
            const libsiedler2::ArchivItem_Map_Header* header = &(dynamic_cast<const glArchivItem_Map*>(map.get(0))->getHeader());
            RTTR_Assert(header);

            serverconfig.playercount = header->getPlayer();
            mapinfo.title = cvStringToUTF8(header->getName());
        } break;
        // Gespeichertes Spiel
        case MAPTYPE_SAVEGAME:
        {
            Savegame save;

            if(!save.Load(mapinfo.filepath, false, false))
                return false;

            // Spieleranzahl
            serverconfig.playercount = save.GetPlayerCount();
            bfs::path mapPath = map_path;
            mapinfo.title = save.mapName;
        } break;
    }

    // Von Lobby abhängig? Dann der Bescheid sagen und auf eine Antwort warten, dass wir den Server
    // erstellen dürfen
    if(serverconfig.servertype == ServerType::LOBBY)
    {
        LOBBYCLIENT.AddServer(serverconfig.gamename, mapinfo.title, (serverconfig.password.length() != 0), serverconfig.port);
        return true;
    }
    else
        // ansonsten können wir sofort starten
        return Start();
}

bool GameServer::Start()
{
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
    for(unsigned i = 0; i < serverconfig.playercount; ++i)
        players.push_back(GameServerPlayer(i));

    // Potentielle KI-Player anlegen
    ai_players.resize(serverconfig.playercount);

    bool host_found = false;

    //// Spieler 0 erstmal der Host
    switch(mapinfo.type)
    {
        default:
            break;

        case MAPTYPE_OLDMAP:
        {
            // Host bei normalen Spieler der erste Spieler
            players[0].is_host = true;

            // Standardeinstellungen aus den SETTINGS für die Addons laden
            //GAMECLIENT.GetGGS().LoadSettings();
            GAMECLIENT.LoadGGS();
            ggs_ = GAMECLIENT.GetGGS();
        } break;
        case MAPTYPE_SAVEGAME:
        {
            Savegame save;
            if(!save.Load(mapinfo.filepath, true, false))
                return false;

            // Bei Savegames die Originalspieldaten noch mit auslesen
            for(unsigned char i = 0; i < serverconfig.playercount; ++i)
            {
                // PlayerState
                const SavedFile::Player& savePlayer = save.GetPlayer(i);
                players[i].ps = PlayerState(savePlayer.ps);

                if(players[i].ps != PS_LOCKED)
                {
                    // (ehemaliger) Spielername
                    players[i].origin_name = savePlayer.name;

                    // Volk, Team und Farbe
                    players[i].nation = savePlayer.nation;
                    players[i].color = savePlayer.color;
                    players[i].team = Team(savePlayer.team);

                }

                // Besetzt --> freigeben, damit auch jemand reinkann
                if(players[i].ps == PS_OCCUPIED)
                {
                    players[i].ps = PS_FREE;
                    // Erster richtiger Spieler? Dann ist das der Host später
                    if(!host_found)
                    {
                        players[i].is_host = true;
                        host_found = true;
                    }
                }
                // KI-Spieler? Namen erzeugen und typ finden
                //warning: if you ever add new ai types - it is not enough that the server knows about the ai! when the host joins his server he will get ONMSPLAYERLIST which also doesnt include the aitype!
                else if(players[i].ps == PS_KI)
                {
                    if(!strncmp(savePlayer.name.c_str(), "Computer", 7))
                    {
                        LOG.lprintf("loading aijh: %s \n", savePlayer.name.c_str());
                        players[i].aiInfo = AI::Info(AI::DEFAULT);
                        players[i].rating = 666;
                    }
                    else
                    {
                        LOG.lprintf("loading default - dummy: %s \n", savePlayer.name.c_str());
                        players[i].aiInfo = AI::Info(AI::DUMMY);
                    }
                    players[i].name = savePlayer.name;
                }
            }

            // Einstellungen aus dem Savegame für die Addons werden in Load geladen

            // Und die GGS
            ggs_ = save.ggs;
        } break;
    }


    // ab in die Konfiguration
    status = SS_CONFIG;

    // und das socket in listen-modus schicken
    if(!serversocket.Listen(serverconfig.port, serverconfig.ipv6, serverconfig.use_upnp))
    {
        LOG.lprintf("GameServer::Start: ERROR: Listening on port %d failed!\n", serverconfig.port);
        LOG.getlasterror("Fehler");
        return false;
    }

    // Zu sich selbst connecten als Host
    if(!GAMECLIENT.Connect("localhost", serverconfig.password, serverconfig.servertype, serverconfig.port, true, serverconfig.ipv6))
        return false;

// clear async logs if necessary

    async_player1_log.clear();
    async_player2_log.clear();

    if (serverconfig.servertype == ServerType::LAN)
    {
        lanAnnouncer.Start();
    }
    AnnounceStatusChange();

    return true;
}

unsigned GameServer::GetFilledSlots() const
{
    unsigned numFilled = 0;
    for (unsigned char cl = 0; cl < serverconfig.playercount; ++cl)
    {
        const GameServerPlayer& player = players[cl];
        if (player.ps != PS_FREE)
            ++numFilled;
    }
    return numFilled;
}

void GameServer::AnnounceStatusChange()
{
    if (serverconfig.servertype == ServerType::LAN)
    {
        LanGameInfo info;
        info.name = serverconfig.gamename;
        info.hasPwd = !serverconfig.password.empty();
        info.map = mapinfo.title;
        info.curPlayer = GetFilledSlots();
        info.maxPlayer = serverconfig.playercount;
        info.port = serverconfig.port;
        info.isIPv6 = serverconfig.ipv6;
        info.version = GetWindowVersion();
        Serializer ser;
        info.Serialize(ser);
        lanAnnouncer.SetPayload(ser.GetData(), ser.GetLength());
    }
    else if (serverconfig.servertype == ServerType::LOBBY)
    {
        LOBBYCLIENT.UpdateServerPlayerCount(GetFilledSlots(), serverconfig.playercount);
    }
}

///////////////////////////////////////////////////////////////////////////////
// Hauptschleife
void GameServer::Run()
{
    if(status == SS_STOPPED)
        return;

    // auf tote Clients prüfen
    ClientWatchDog();

    // auf neue Clients warten
    if(status == SS_CONFIG)
        WaitForClients();
    else if(status == SS_GAME)
        ExecuteGameFrame();

    // post zustellen
    FillPlayerQueues();

    if(countdown.do_countdown)
    {
        // countdown erzeugen
        if(VIDEODRIVER.GetTickCount() - countdown.lasttime > 1000)
        {
            // nun echt starten
            if(countdown.countdown < 0)
            {
                countdown.Clear();
                if (!StartGame())
                {
                    GAMEMANAGER.ShowMenu();
                    return;
                }
            }
            else
            {
                SendToAll(GameMessage_Server_Countdown(countdown.countdown));
                LOG.write("SERVER >>> BROADCAST: NMS_SERVER_COUNTDOWN(%d)\n", countdown.countdown);

                countdown.lasttime = VIDEODRIVER.GetTickCount();

                --countdown.countdown;
            }
        }
    }

    // queues abarbeiten
    for(unsigned int client = 0; client < serverconfig.playercount; ++client)
    {
        GameServerPlayer& player = players[client];

        // maximal 10 Pakete verschicken
        player.send_queue.send(player.so, 10);

        // recv-queue abarbeiten
        while(player.recv_queue.count() > 0)
        {
            player.recv_queue.front()->run(this, client);
            player.recv_queue.pop();
        }
    }

    lanAnnouncer.Run();
}

///////////////////////////////////////////////////////////////////////////////
// stoppt den server
void GameServer::Stop()
{
    // player verabschieden
    players.clear();

    // aufräumen
    framesinfo.Clear();
    serverconfig.Clear();
    mapinfo.Clear();
    countdown.Clear();

    // KI-Player zerstören
    for(unsigned i = 0; i < ai_players.size(); ++i)
        delete ai_players[i];
    ai_players.clear();

    // laden dicht machen
    serversocket.Close();
    // clear jump target
    skiptogf=0;

    lanAnnouncer.Stop();

    if(status != SS_STOPPED)
        LOG.lprintf("server state changed to stop\n");    

    // status
    status = SS_STOPPED;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  startet den Spielstart-Countdown
 *
 *  @author FloSoft
 */
bool GameServer::StartCountdown()
{
    unsigned char client = 0xFF;

    int playerCount = 0;

    // Alle Spieler da?
    for(client = 0; client < serverconfig.playercount; ++client)
    {
        GameServerPlayer& player = players[client];

        // noch nicht alle spieler da -> feierabend!
        if( (player.ps == PS_FREE) || (player.ps == PS_RESERVED) )
            return false;
        else if(player.ps != PS_LOCKED && player.ps != PS_KI && !player.ready)
            return false;
        if(player.ps == PS_OCCUPIED)
            playerCount++;
    }

    std::set<unsigned> takenColors;

    // Check all players have different colors
    for(unsigned p = 0; p < serverconfig.playercount; ++p)
    {
        GameServerPlayer& player = players[p];
        if(player.isUsed())
        {
            if(helpers::contains(takenColors, player.color))
                return false;
            takenColors.insert(player.color);
        }
    }

    // Countdown starten (except its single player)
    countdown.Clear((playerCount > 1) ? 2 : -1);
    countdown.do_countdown = true;

    return true;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  stoppt den Spielstart-Countdown
 *
 *  @author FloSoft
 */
void GameServer::CancelCountdown()
{
    // Countdown-Stop allen mitteilen
    countdown.Clear();
    SendToAll(GameMessage_Server_CancelCountdown());
    LOG.write("SERVER >>> BROADCAST: NMS_SERVER_CANCELCOUNTDOWN\n");
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  startet das Spiel.
 *
 *  @author OLiver
 *  @author FloSoft
 */
bool GameServer::StartGame()
{
    lanAnnouncer.Stop();

    // Bei Savegames wird der Startwert von den Clients aus der Datei gelesen!
    unsigned random_init = (mapinfo.type == MAPTYPE_SAVEGAME) ? 0xFFFFFFFF : VIDEODRIVER.GetTickCount();

    // Höchsten Ping ermitteln
    unsigned highest_ping = 0;
    unsigned char client = 0xFF;

    for(client = 0; client < serverconfig.playercount; ++client)
    {
        GameServerPlayer& player = players[client];

        if(player.ps == PS_OCCUPIED)
        {
            if(player.ping > highest_ping)
                highest_ping = player.ping;
        }
    }

    framesinfo.gfLenghtNew = framesinfo.gfLenghtNew2 = framesinfo.gf_length = SPEED_GF_LENGTHS[ggs_.game_speed];

    // NetworkFrame-Länge bestimmen, je schlechter (also höher) die Pings, desto länger auch die Framelänge
    unsigned i = 1;
    for( ; i < 20; ++i)
    {
        if(i * framesinfo.gf_length > highest_ping + 200)
            break;
    }

    framesinfo.nwf_length = i;

    // Mond malen
    LOADER.GetImageN("resource", 33)->Draw(VIDEODRIVER.GetMouseX(), VIDEODRIVER.GetMouseY() - 40, 0, 0, 0, 0, 0, 0);
    VIDEODRIVER.SwapBuffers();

    GameMessage_Server_Start start_msg(random_init, framesinfo.nwf_length);

    LOG.lprintf("SERVER: Using gameframe length of %dms\n", framesinfo.gf_length);
    LOG.lprintf("SERVER: Using networkframe length of %u GFs (%ums)\n", framesinfo.nwf_length, framesinfo.nwf_length * framesinfo.gf_length);

    // Spielstart allen mitteilen
    SendToAll(start_msg);
    LOG.write("SERVER >>> BROADCAST: NMS_SERVER_START(%d)\n", random_init);

    framesinfo.lastTime = VIDEODRIVER.GetTickCount();

    try
    {
        // GameClient soll erstmal starten, damit wir von ihm die benötigten Daten für die KIs bekommen
        GAMECLIENT.StartGame(random_init);
    }
    catch (SerializedGameData::Error& error)
    {
        LOG.lprintf("Error when loading game: %s\n", error.what());
        return false;
    }

    // read back gf_nr (if savegame)
    if(mapinfo.type == MAPTYPE_SAVEGAME)
        framesinfo.gf_nr = GAMECLIENT.GetGFNumber();

    // Erste KI-Nachrichten schicken
    for(unsigned i = 0; i < this->serverconfig.playercount; ++i)
    {
        if(players[i].ps == PS_KI)
        {
            SendNothingNC(i);
            ai_players[i] = GAMECLIENT.CreateAIPlayer(i);
        }
    }

    LOG.write("SERVER >>> BROADCAST: NMS_NWF_DONE\n");

    // Spielstart allen mitteilen
    SendToAll(GameMessage_Server_NWFDone(0xff, framesinfo.gf_nr, framesinfo.gf_length, true));

    // ab ins game wechseln
    status = SS_GAME;

    return true;
}

///////////////////////////////////////////////////////////////////////////////
// wechselt Spielerstatus durch
void GameServer::TogglePlayerState(unsigned char client)
{
    GameServerPlayer&  player = players[client];

    // oh ein spieler, weg mit ihm!
    if(player.ps == PS_OCCUPIED)
    {
        KickPlayer(client, NP_NOCAUSE, 0);
        return;
    }

    // playerstatus weiterwechseln
    switch(player.ps)
    {
        default: break;
        case PS_FREE:
        {
            player.ps = PS_KI;
            player.aiInfo = AI::Info(AI::DEFAULT, AI::EASY);
            // Baby mit einem Namen Taufen ("Name (KI)")
            SetAIName(client);
        } break;
        case PS_KI:
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
                    SetAIName(client);
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
    player.ready = (player.ps == PS_KI);

    // Tat verkünden
    SendToAll(GameMessage_Player_Set_State(client, player.ps, player.aiInfo));
    AnnounceStatusChange();

    // If slot is filled, check current color
    if(player.isUsed())
        CheckAndSetColor(client, player.color);
}

///////////////////////////////////////////////////////////////////////////////
// Team der KI ändern
void GameServer::TogglePlayerTeam(unsigned char client)
{
    GameServerPlayer& player = players[client];

    // nur KI
    if(player.ps != PS_KI)
        return;

    // team wechseln
    //random team special case
    //switch from random team?
    if(player.team == TM_RANDOMTEAM || player.team == TM_RANDOMTEAM2 || player.team == TM_RANDOMTEAM3 || player.team == TM_RANDOMTEAM4)
        OnGameMessage(GameMessage_Player_Set_Team(client, Team((TM_RANDOMTEAM + 1) % TEAM_COUNT)));
    else
    {
        if(player.team == TM_NOTEAM) //switch to random team?
        {
            int rand = RANDOM.Rand(__FILE__, __LINE__, 0, 4);
            switch(rand)
            {
                case 0:
                    OnGameMessage(GameMessage_Player_Set_Team(client, TM_RANDOMTEAM));
                    break;
                case 1:
                    OnGameMessage(GameMessage_Player_Set_Team(client, TM_RANDOMTEAM2));
                    break;
                case 2:
                    OnGameMessage(GameMessage_Player_Set_Team(client, TM_RANDOMTEAM3));
                    break;
                case 3:
                    OnGameMessage(GameMessage_Player_Set_Team(client, TM_RANDOMTEAM4));
                    break;
                default:
                    OnGameMessage(GameMessage_Player_Set_Team(client, TM_RANDOMTEAM));
                    break;
            }

        }
        else
            OnGameMessage(GameMessage_Player_Set_Team(client, Team((player.team + 1) % TEAM_COUNT)));
    }
}

///////////////////////////////////////////////////////////////////////////////
// Farbe der KI ändern
void GameServer::TogglePlayerColor(unsigned char client)
{
    GameServerPlayer& player = players[client];

    // nur KI
    if(player.ps != PS_KI)
        return;

    CheckAndSetColor(client, PLAYER_COLORS[(player.GetColorIdx() + 1) % PLAYER_COLORS.size()]);
}

///////////////////////////////////////////////////////////////////////////////
// Nation der KI ändern
void GameServer::TogglePlayerNation(unsigned char client)
{
    GameServerPlayer& player = players[client];

    // nur KI5
    if(player.ps != PS_KI)
        return;

    // Nation wechseln
    OnGameMessage(GameMessage_Player_Set_Nation(client, Nation((player.nation + 1) % NAT_COUNT)));
}

///////////////////////////////////////////////////////////////////////////////
// Spieleinstellungen verschicken
void GameServer::ChangeGlobalGameSettings(const GlobalGameSettings& ggs)
{
    this->ggs_ = ggs;
    SendToAll(GameMessage_GGSChange(ggs));
    LOG.write("SERVER >>> BROADCAST: NMS_GGS_CHANGE\n");
}

void GameServer::RemoveLuaScript()
{
    RTTR_Assert(status == SS_CONFIG);
    mapinfo.luaFilepath.clear();
    mapinfo.luaData.Clear();
    mapinfo.luaChecksum = 0;
    SendToAll(GameMessage_RemoveLua());
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Nachricht an Alle
 *
 *  @author FloSoft
 *  @author OLiver
 */
void GameServer::SendToAll(const GameMessage& msg)
{
    for(unsigned int id = 0; id < players.getCount(); ++id)
    {
        GameServerPlayer& player = players[id];

        // ist der Slot Belegt, dann Nachricht senden
        if(player.ps == PS_OCCUPIED)
            player.send_queue.push(msg.duplicate());
    }
}

///////////////////////////////////////////////////////////////////////////////
// kickt einen spieler und räumt auf
void GameServer::KickPlayer(unsigned char playerid, unsigned char cause, unsigned short param)
{
    NS_PlayerKicked npk;
    npk.playerid = playerid;
    npk.cause = cause;
    npk.param = param;

    KickPlayer(npk);
}

void GameServer::KickPlayer(NS_PlayerKicked npk)
{
    GameServerPlayer& player = players[npk.playerid];

    // send-queue flushen
    player.send_queue.flush(player.so);

    PlayerState oldPs = player.ps;

    // töten, falls außerhalb
    if(status == SS_GAME)
    {
        // KI-Spieler muss übernehmen
        player.ps = PS_KI;
        player.aiInfo.type = AI::DUMMY;
        player.aiInfo.level = AI::MEDIUM;
        ai_players[npk.playerid] = GAMECLIENT.CreateAIPlayer(npk.playerid);
        // Und Socket schließen, das brauchen wir nicht mehr
        player.so.Close();
        // Clear queue as this is an AI now (Client executes the GCs, not the server, otherwise we WOULD need to execute those GCs)
        player.gc_queue.clear();
    }
    else
        player.clear();

    // Do not send notifications if the player was not already there
    if(oldPs == PS_RESERVED)
        return;

    SendToAll(GameMessage_Player_Kicked(npk.playerid, npk.cause, npk.param));
    AnnounceStatusChange();
    LOG.write("SERVER >>> BROADCAST: NMS_PLAYERKICKED(%d,%d,%d)\n", npk.playerid, npk.cause, npk.param);

    if(status == SS_GAME)
        SendNothingNC(npk.playerid);
}

///////////////////////////////////////////////////////////////////////////////
// testet, ob in der Verbindungswarteschlange Clients auf Verbindung warten
void GameServer::ClientWatchDog()
{
    SocketSet set;
    set.Clear();

    // sockets zum set hinzufügen
    for(unsigned char client = 0; client < serverconfig.playercount; ++client)
    {
        if( players[client].isHuman() )
        {
            // zum set hinzufügen
            set.Add(players[client].so);
        }
    }

    // auf fehler prüfen
    if(set.Select(0, 2) > 0)
    {
        for(unsigned char client = 0; client < serverconfig.playercount; ++client)
        {
            if(set.InSet(players[client].so))
            {
                LOG.lprintf("SERVER: Error on socket of player %d, bye bye!\n", client);
                KickPlayer(client, NP_CONNECTIONLOST, 0);
            }
        }
    }

    for(unsigned char client = 0; client < serverconfig.playercount; ++client)
    {
        GameServerPlayer& player = players[client];
        // player anpingen
        player.doPing();
        // auf timeout prüfen
        player.doTimeout();
    }
}

void GameServer::ExecuteGameFrame()
{
    RTTR_Assert(status == SS_GAME);

    if(framesinfo.isPaused)
        return;

    unsigned int currentTime = VIDEODRIVER.GetTickCount();

    // prüfen ob GF vergangen
    if(currentTime - framesinfo.lastTime > framesinfo.gf_length || skiptogf > framesinfo.gf_nr)
    {
        // NWF vergangen?
        if(framesinfo.gf_nr % framesinfo.nwf_length==0)
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
            ai_players[i]->RunGF(framesinfo.gf_nr, isNWF);
    }
    ++framesinfo.gf_nr;
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
    checksums.reserve(serverconfig.playercount);

    // Send AI commands and check for asyncs
    for(unsigned char client = 0; client < serverconfig.playercount; ++client)
    {
        GameServerPlayer& player = players[client];

        // Befehle der KI senden
        if(player.ps == PS_KI)
        {
            LOG.write("SERVER >>> GC %u\n", client);
            SendToAll(GameMessage_GameCommand(client, AsyncChecksum(0), ai_players[client]->GetGameCommands()));
            ai_players[client]->FetchGameCommands();
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
        checksums.push_back(curChecksum.randState);

        // Checksumme des ersten Spielers als Richtwert
        if (referencePlayerIdx == 0xFF)
        {
            referenceChecksum = curChecksum;
            referencePlayerIdx = client;
        }

        player.gc_queue.pop_front();
        RTTR_Assert(player.gc_queue.size() <= 1); // At most 1 additional GC-Message, otherwise the client skipped a NWF

        // Checksummen nicht gleich?
        if (curChecksum != referenceChecksum)
        {
            LOG.lprintf("Async at GF %u of players %u vs %u: Checksum %i:%i ObjCt %u:%u ObjIdCt %u:%u\n", framesinfo.gf_nr, client, referencePlayerIdx,
                curChecksum.randState, referenceChecksum.randState,
                curChecksum.objCt, referenceChecksum.objCt,
                curChecksum.objIdCt, referenceChecksum.objIdCt);

            // AsyncLog der asynchronen Player anfordern
            if (async_player1 == -1)
            {
                GameServerPlayer& refPlayer = players[referencePlayerIdx];
                async_player1 = refPlayer.getPlayerID();
                async_player1_done = false;
                refPlayer.send_queue.push(new GameMessage_GetAsyncLog(async_player1));
                refPlayer.send_queue.flush(refPlayer.so);

                async_player2 = client;
                async_player2_done = false;
                player.send_queue.push(new GameMessage_GetAsyncLog(client));
                player.send_queue.flush(player.so);

                // Async-Meldung rausgeben.
                SendToAll(GameMessage_Server_Async(checksums));

                // Spiel pausieren
                RTTR_Assert(!framesinfo.isPaused);
                TogglePause();
            }
        }
    }

    if(framesinfo.gfLenghtNew != framesinfo.gf_length)
    {
        unsigned oldGfLen = framesinfo.gf_length;
        unsigned oldnNwfLen = framesinfo.nwf_length;
        framesinfo.ApplyNewGFLength();

        LOG.lprintf("Server %d/%d: Speed changed from %d to %d. NWF %u to %u\n", framesinfo.gf_nr, framesinfo.gf_nr, oldGfLen, framesinfo.gf_length, oldnNwfLen, framesinfo.nwf_length);
    }

    framesinfo.gfLenghtNew = framesinfo.gfLenghtNew2;

    SendToAll(GameMessage_Server_NWFDone(0xff, framesinfo.gf_nr, framesinfo.gfLenghtNew));
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
        LOG.lprintf("SERVER: Kicke Spieler %d in %u Sekunden\n", playerIdx, timeOut);
}

unsigned char GameServer::GetLaggingPlayer() const
{
    for(unsigned char client = 0; client < serverconfig.playercount; ++client)
    {
        const GameServerPlayer& player = players[client];

        if(player.ps == PS_OCCUPIED && player.gc_queue.empty())
        {
            // der erste, der erwischt wird, bekommt den zuschlag ;-)
            return client;
        }
    }
    return 0xFF;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Sendet ein NC-Paket ohne Befehle.
 *
 *  @author OLiver
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

            unsigned char playerid = 0xFF;
            // Geeigneten Platz suchen
            for(unsigned int client = 0; client < serverconfig.playercount; ++client)
            {
                if(players[client].ps == PS_FREE)
                {
                    // platz reservieren
                    players[client].reserve(socket, client);
                    playerid = client;
                    //LOG.lprintf("new socket, about to tell him about his playerid: %i \n",playerid);
                    // schleife beenden
                    break;
                }
            }

            GameMessage_Player_Id msg(playerid);
            MessageHandler::send(socket, msg);

            // war kein platz mehr frei, wenn ja dann verbindung trennen?
            if(playerid == 0xFF)
                socket.Close();
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
// füllt die warteschlangen mit "paketen"
void GameServer::FillPlayerQueues()
{
    SocketSet set;
    unsigned char client = 0xFF;
    bool not_empty = false;

    // erstmal auf Daten überprüfen
    do
    {
        // sockets zum set hinzufügen
        for(client = 0; client < serverconfig.playercount; ++client)
        {
            if( players[client].isHuman() )
            {
                // zum set hinzufügen
                set.Add(players[client].so);
            }
        }

        not_empty = false;

        // ist eines der Sockets im Set lesbar?
        if(set.Select(0, 0) > 0)
        {
            for(client = 0; client < serverconfig.playercount; ++client)
            {
                if( players[client].isHuman() && set.InSet(players[client].so) )
                {
                    // nachricht empfangen
                    if(!players[client].recv_queue.recv(players[client].so))
                    {
                        LOG.lprintf("SERVER: Receiving Message for player %d failed, kick it like Beckham!\n", client);
                        KickPlayer(client, NP_CONNECTIONLOST, 0);
                    }

                    if(players[client].ps == PS_OCCUPIED)
                        not_empty = true;
                }
            }
        }
    }
    while(not_empty);
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
    //LOG.write("SERVER >>> BROADCAST: NMS_PLAYER_PING(%d,%u)\n", client, *ping);
}

///////////////////////////////////////////////////////////////////////////////
// servertype
inline void GameServer::OnGameMessage(const GameMessage_Server_Type& msg)
{
    if(msg.player >= GetMaxPlayerCount())
        return;

    GameServerPlayer& player = players[msg.player];

    int typok = 0;
    if(msg.type != serverconfig.servertype)
        typok = 1;
    else if(msg.version != GetWindowVersion())
        typok = 2;

    player.send_queue.push(new GameMessage_Server_TypeOK(typok));

    if(typok != 0)
        KickPlayer(msg.player, NP_CONNECTIONLOST, 0);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Server-Passwort-Nachricht
 *
 *  @author FloSoft
 */
void GameServer::OnGameMessage(const GameMessage_Server_Password& msg)
{
    if(msg.player >= GetMaxPlayerCount())
        return;

    GameServerPlayer& player = players[msg.player];

    std::string passwordok = (serverconfig.password == msg.password ? "true" : "false");

    player.send_queue.push(new GameMessage_Server_Password(passwordok));

    if(passwordok == "false")
        KickPlayer(msg.player, NP_WRONGPASSWORD, 0);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Server-Chat-Nachricht.
 *
 *  @author FloSoft
 *  @author OLiver
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

    LOG.write("CLIENT%d >>> SERVER: NMS_PLAYER_NAME(%s)\n", msg.player, msg.playername.c_str());

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

    LOG.write("CLIENT%d >>> SERVER: NMS_PLAYER_TOGGLENATION\n", msg.player);

    // Nation-Change senden
    SendToAll(GameMessage_Player_Set_Nation(msg.player, msg.nation));

    LOG.write("SERVER >>> BROADCAST: NMS_PLAYER_TOGGLENATION(%d, %d)\n", msg.player, player.nation);
}

///////////////////////////////////////////////////////////////////////////////
// Team weiterwechseln
inline void GameServer::OnGameMessage(const GameMessage_Player_Set_Team& msg)
{
    if(msg.player >= GetMaxPlayerCount())
        return;

    GameServerPlayer& player = players[msg.player];

    player.team = msg.team;

    LOG.write("CLIENT%d >>> SERVER: NMS_PLAYER_TOGGLETEAM\n", msg.player);
    SendToAll(GameMessage_Player_Set_Team(msg.player, msg.team));
    LOG.write("SERVER >>> BROADCAST: NMS_PLAYER_TOGGLETEAM(%d, %d)\n", msg.player, player.team);
}

///////////////////////////////////////////////////////////////////////////////
// Farbe weiterwechseln
inline void GameServer::OnGameMessage(const GameMessage_Player_Set_Color& msg)
{
    if(msg.player >= GetMaxPlayerCount())
        return;

    LOG.write("CLIENT%u >>> SERVER: NMS_PLAYER_TOGGLECOLOR %u\n", msg.player, msg.color);
    CheckAndSetColor(msg.player, msg.color);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Spielerstatus wechseln
 *
 *  @author FloSoft
 */
inline void GameServer::OnGameMessage(const GameMessage_Player_Ready& msg)
{
    if(msg.player >= GetMaxPlayerCount())
        return;

    GameServerPlayer& player = players[msg.player];

    player.ready = msg.ready;

    // countdown ggf abbrechen
    if(!player.ready && countdown.do_countdown)
        CancelCountdown();

    LOG.write("CLIENT%d >>> SERVER: NMS_PLAYER_READY(%s)\n", msg.player, (player.ready ? "true" : "false"));
    // Ready-Change senden
    SendToAll(GameMessage_Player_Ready(msg.player, msg.ready));
    LOG.write("SERVER >>> BROADCAST: NMS_PLAYER_READY(%d, %s)\n", msg.player, (player.ready ? "true" : "false"));
}

///////////////////////////////////////////////////////////////////////////////
// Checksumme
inline void GameServer::OnGameMessage(const GameMessage_Map_Checksum& msg)
{
    if(msg.player >= GetMaxPlayerCount())
        return;

    GameServerPlayer& player = players[msg.player];

    bool checksumok = (msg.mapChecksum == mapinfo.mapChecksum && msg.luaChecksum == mapinfo.luaChecksum);

    LOG.write("CLIENT%d >>> SERVER: NMS_MAP_CHECKSUM(%u) expected: %u, ok: %s\n", msg.player, msg.mapChecksum, mapinfo.mapChecksum, checksumok ? "yes" : "no");

    // Antwort senden
    player.send_queue.push(new GameMessage_Map_ChecksumOK(checksumok));

    LOG.write("SERVER >>> CLIENT%d: NMS_MAP_CHECKSUM(%d)\n", msg.player, checksumok);

    if(!checksumok)
        KickPlayer(msg.player, NP_WRONGCHECKSUM, 0);
    else
    {
        // den anderen Spielern mitteilen das wir einen neuen haben
        SendToAll(GameMessage_Player_New(msg.player, player.name));

        LOG.write("SERVER >>> BROADCAST: NMS_PLAYER_NEW(%d, %s)\n", msg.player, player.name.c_str());

        // belegt markieren
        player.ps = PS_OCCUPIED;
        player.lastping = VIDEODRIVER.GetTickCount();

        // Servername senden
        player.send_queue.push(new GameMessage_Server_Name(serverconfig.gamename));

        // Spielerliste senden
        player.send_queue.push(new GameMessage_Player_List(players));

        // Assign unique color
        CheckAndSetColor(msg.player, player.color);

        // GGS senden
        player.send_queue.push(new GameMessage_GGSChange(ggs_));

        AnnounceStatusChange();

        LOG.write("SERVER >>> BROADCAST: NMS_GGS_CHANGE\n");
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
    LOG.write("SERVER <<< GC %u\n", msg.player);

    // Only valid from humans (for now)
    if(player.ps != PS_OCCUPIED)
        return;

    //// Command schließlich an alle Clients weiterleiten, aber nicht wenn derjenige Spieler besiegt wurde!
    if(!player.isDefeated())
    {
        // NFCs speichern
        player.gc_queue.push_back(msg);
        SendToAll(msg);
    }else
    {
        GameMessage_GameCommand emptyMsg = msg;
        emptyMsg.gcs.clear();
        player.gc_queue.push_back(emptyMsg);
        SendToAll(emptyMsg);
    }
}

void GameServer::OnGameMessage(const GameMessage_SendAsyncLog& msg)
{
    if (msg.player == async_player1)
    {
        async_player1_log.insert(async_player1_log.end(), msg.entries.begin(), msg.entries.end());

        if (msg.last)
        {
            LOG.lprintf("Received async logs from %u (%lu entries).\n", async_player1, async_player1_log.size());
            async_player1_done = true;
        }
    }
    else if (msg.player == async_player2)
    {
        async_player2_log.insert(async_player2_log.end(), msg.entries.begin(), msg.entries.end());

        if (msg.last)
        {
            LOG.lprintf("Received async logs from %u (%lu entries).\n", async_player2, async_player2_log.size());
            async_player2_done = true;
        }
    }
    else
    {
        LOG.lprintf("Received async log from %u, but did not expect it!\n", msg.player);
        return;
    }

    // list is not yet complete, keep it coming...
    if (!async_player1_done || !async_player2_done)
        return;

    LOG.lprintf("Async logs received completely.\n");

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

    LOG.lprintf("There are %u identical async log entries.\n", identical);

    if (SETTINGS.global.submit_debug_data == 1
#ifdef _WIN32
            || (MessageBoxA(NULL,
                            _("The game clients are out of sync. Would you like to send debug information to RttR to help us avoiding this in the future? Thank you very much!"),
                            _("Error"), MB_YESNO | MB_ICONERROR | MB_TASKMODAL | MB_SETFOREGROUND) == IDYES)
#endif
       )
    {
        DebugInfo di;
        LOG.lprintf("Sending async logs %s.\n",
                    di.SendAsyncLog(it1, it2, async_player1_log, async_player2_log, identical) ? "succeeded" : "failed");

        di.SendReplay();
    }

    std::string fileName = GetFilePath(FILE_PATHS[47]) + TIME.FormatTime("async_%Y-%m-%d_%H-%i-%s") + "Server.log";

    // open async log
    FILE* file = fopen(fileName.c_str(), "w");

    if (file)
    {
        // print identical lines, they help in tracing the bug
        for(unsigned i = 0; i < identical; i++)
        {
            fprintf(file, "[I]: %u:R(%d)=%d,z=%d | %s#%u|id=%u\n", it1->counter, it1->max, it1->GetValue(), it1->rngState, it1->src_name.c_str(), it1->src_line, it1->obj_id);
            
            ++it1; ++it2;
        }

        while ((it1 != async_player1_log.end()) && (it2 != async_player2_log.end()))
        {
            fprintf(file, "[S]: %u:R(%d)=%d,z=%d | %s#%u|id=%u\n", it1->counter, it1->max, it1->GetValue(), it1->rngState, it1->src_name.c_str(), it1->src_line, it1->obj_id);
            fprintf(file, "[C]: %u:R(%d)=%d,z=%d | %s#%u|id=%u\n", it2->counter, it2->max, it2->GetValue(), it2->rngState, it2->src_name.c_str(), it2->src_line, it2->obj_id);

            ++it1; ++it2;
        }

        fclose(file);

        LOG.lprintf("Async log saved at \"%s\"\n", fileName.c_str());
    }
    else
    {
        LOG.lprintf("Failed to save async log at \"%s\"\n", fileName.c_str());
    }

    async_player1_log.clear();
    async_player2_log.clear();

    KickPlayer(msg.player, NP_ASYNC, 0);
}

void GameServer::CheckAndSetColor(unsigned playerIdx, unsigned newColor)
{
    RTTR_Assert(playerIdx < serverconfig.playercount);
    RTTR_Assert(serverconfig.playercount <= PLAYER_COLORS.size()); // Else we may not find a valid color!

    GameServerPlayer& player = players[playerIdx];
    RTTR_Assert(player.isUsed()); // Should only set colors for taken spots

    // Get colors used by other players
    std::set<unsigned> takenColors;
    for(unsigned p = 0; p < serverconfig.playercount; ++p)
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
    LOG.write("SERVER >>> BROADCAST: NMS_PLAYER_TOGGLECOLOR(%d, %d)\n", playerIdx, player.color);
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
    if(players[msg.player].ps != PS_OCCUPIED || players[msg.player2].ps != PS_KI)
        return;
    SendToAll(msg);
    ChangePlayer(msg.player, msg.player2);
}

void GameServer::ChangePlayer(const unsigned char old_id, const unsigned char new_id)
{
    LOG.lprintf("GameServer::ChangePlayer %i - %i \n",old_id, new_id);
    using std::swap;
    swap(players[new_id].ps, players[old_id].ps);
    swap(players[new_id].so, players[old_id].so);
    swap(players[new_id].send_queue, players[old_id].send_queue);
    swap(players[new_id].recv_queue, players[old_id].recv_queue);
    swap(players[new_id].aiInfo, players[old_id].aiInfo);

    // Alte KI löschen
    delete ai_players[new_id];
    ai_players[new_id] = NULL;

    ai_players[old_id] = GAMECLIENT.CreateAIPlayer(old_id);

    //swap the gamecommand queue
    swap(players[old_id].gc_queue, players[new_id].gc_queue);
}

bool GameServer::TogglePause()
{
    framesinfo.isPaused = !framesinfo.isPaused;
    SendToAll(GameMessage_Pause(framesinfo.isPaused, framesinfo.gf_nr + framesinfo.nwf_length));

    return framesinfo.isPaused;
}

void GameServer::SwapPlayer(const unsigned char player1, const unsigned char player2)
{
    // Message verschicken
    SendToAll(GameMessage_Player_Swap(player1, player2));
    // Spieler vertauschen
    players[player1].SwapInfo(players[player2]);
}

void GameServer::SetAIName(const unsigned player_id)
{
    // Baby mit einem Namen Taufen ("Name (KI)")
    char str[128];
    GameServerPlayer& player = players[player_id];
    if (player.aiInfo.type == AI::DUMMY)
        sprintf(str, _("Dummy %u"), unsigned(player_id));
    else
        sprintf(str, _("Computer %u"), unsigned(player_id));

    player.name = str;
    player.name += _(" (AI)");

    if (player.aiInfo.type == AI::DEFAULT)
    {
        switch(player.aiInfo.level)
        {
        case AI::EASY:
            player.name += _(" (easy)");
            break;
        case AI::MEDIUM:
            player.name += _(" (medium)");
            break;
        case AI::HARD:
            player.name += _(" (hard)");
            break;
        }
    }

}

bool GameServer::SendAIEvent(AIEvent::Base* ev, unsigned receiver)
{
    if(ai_players[receiver])
    {
        ai_players[receiver]->SendAIEvent(ev);
        return true;
    }else
    {
        delete ev;
        return false;
    }
}
