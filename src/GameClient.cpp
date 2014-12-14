// $Id: GameClient.cpp 9548 2014-12-14 19:51:50Z marcus $
//
// Copyright (c) 2005 - 2011 Settlers Freaks (sf-team at siedler25.org)
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
#include "main.h"
#include "GameClient.h"

#include "GameManager.h"
#include "GameMessages.h"

#include "GlobalVars.h"
#include "SocketSet.h"
#include "Loader.h"
#include "Settings.h"
#include "FileChecksum.h"
#include "VideoDriverWrapper.h"
#include "WindowManager.h"
#include "dskGameInterface.h"
#include "Random.h"
#include "GameServer.h"
#include "GameWorld.h"
#include "EventManager.h"
#include "GameObject.h"
#include "files.h"
#include "GlobalGameSettings.h"

#include "SerializedGameData.h"
#include "LobbyClient.h"
#include "Settings.h"
#include "files.h"
#include "ClientInterface.h"
#include "GameCommands.h"
#include "AIPlayer.h"
#include "AIPlayerJH.h"

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#if defined _WIN32 && defined _DEBUG && defined _MSC_VER
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

///////////////////////////////////////////////////////////////////////////////
/*
 *
 *
 *  @author
 */
void GameClient::ClientConfig::Clear()
{
    server = "";
    gamename = "";
    password = "";
    mapfile = "";
    mapfilepath = "";
    servertyp = 0;
    port = 0;
    host = false;
}

///////////////////////////////////////////////////////////////////////////////
/*
 *
 *
 *  @author
 */
void GameClient::MapInfo::Clear()
{
    map_type = MAPTYPE_OLDMAP;
    partcount = 0;
    ziplength = 0;
    length = 0;
    checksum = 0;
    title = "";
    delete [] zipdata;
    zipdata = 0;
}

///////////////////////////////////////////////////////////////////////////////
/*
 *
 *
 *  @author
 */
void GameClient::FramesInfo::Clear()
{
    nr = 0;
    gf_length = 0;
    gf_length_new = 0;
    nwf_length = 0;
    frame_time = 0;
    lasttime = 0;
    lastmsgtime = 0;
    pausetime = 0;
    pause = false;
	pause_gf = 0;
}

///////////////////////////////////////////////////////////////////////////////
/*
 *
 *
 *  @author
 */
void GameClient::RandCheckInfo::Clear()
{
    rand = 0;
}

///////////////////////////////////////////////////////////////////////////////
/*
 *
 *
 *  @author
 */
void GameClient::ReplayInfo::Clear()
{
    async = 0;
    end = false;
    next_gf = 0;
    filename = "";
    all_visible = false;
}

///////////////////////////////////////////////////////////////////////////////
/*
 *
 *
 *  @author FloSoft
 */
GameClient::GameClient(void)
    : recv_queue(&GameMessage::create_game), send_queue(&GameMessage::create_game),
      ci(NULL)
{
    clientconfig.Clear();
    framesinfo.Clear();
    randcheckinfo.Clear();
    postMessages.clear();
	skiptogf=0;
}

///////////////////////////////////////////////////////////////////////////////
/*
 *
 *
 *  @author FloSoft
 */
GameClient::~GameClient(void)
{
    Stop();
    ExitGame();
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Verbindet den Client mit einem Server
 *
 *  @param server    Hostname des Zielrechners
 *  @param password  Passwort des Spieles
 *  @param servertyp Servertyp des Spieles (Direct/LAN/usw)
 *  @param host      gibt an ob wir selbst der Host sind
 *
 *  @return true, wenn Client erfolgreich verbunden und gestartet
 *
 *  @author OLiver
 *  @author FloSoft
 */
bool GameClient::Connect(const std::string& server, const std::string& password, unsigned char servertyp, unsigned short port, bool host, bool use_ipv6)
{
    Stop();
    ggs.LoadSettings();

    // Name und Password kopieren
    clientconfig.server = server;
    clientconfig.password = password;

    clientconfig.servertyp = servertyp;
    clientconfig.port = port;
    clientconfig.host = host;

    // Verbinden
    if(!socket.Connect(server.c_str(), port, use_ipv6, (Socket::PROXY_TYPE)SETTINGS.proxy.typ, SETTINGS.proxy.proxy, SETTINGS.proxy.port))
    {
        LOG.lprintf("GameClient::Connect: ERROR: Connect failed!\n");
        return false;
    }

    state = CS_CONNECT;

    if(ci)
        ci->CI_NextConnectState(CS_WAITFORANSWER);

    // Es wird kein Replay abgespielt, sondern dies ist ein richtiges Spiel
    replay_mode = false;

    return true;
}

///////////////////////////////////////////////////////////////////////////////
/*
 *  Hauptschleife des Clients
 *
 *  @author FloSoft
 */
void GameClient::Run()
{
    if(state == CS_STOPPED)
        return;

    SocketSet set;

    // erstmal auf Daten überprüfen
    set.Clear();

    // zum set hinzufügen
    set.Add(socket);
    if(set.Select(0, 0) > 0)
    {
        // nachricht empfangen
        if(!recv_queue.recv(&socket))
        {
            LOG.lprintf("Receiving Message from server failed\n");
            ServerLost();
        }
    }

    // nun auf Fehler prüfen
    set.Clear();

    // zum set hinzufügen
    set.Add(socket);

    // auf fehler prüfen
    if(set.Select(0, 2) > 0)
    {
        if(set.InSet(socket))
        {
            // Server ist weg
            LOG.lprintf("Error on socket to server\n");
            ServerLost();
        }
    }

    if(state == CS_GAME)
        ExecuteGameFrame();

    // maximal 10 Pakete verschicken
    send_queue.send(&socket, 10);

    // recv-queue abarbeiten
    while(recv_queue.count() > 0)
    {
        recv_queue.front()->run(this, 0xFFFFFFFF);
        recv_queue.pop();
    }
}

///////////////////////////////////////////////////////////////////////////////
/*
 *  Stoppt das Spiel
 *
 *  @author FloSoft
 */
void GameClient::Stop()
{
    if(state != CS_STOPPED)
    {
        if(LOBBYCLIENT.LoggedIn()) // steht die Lobbyverbindung noch?
            LOBBYCLIENT.DeleteServer();

        LOG.lprintf("client state changed to stop\n");
    }

    // Nicht im Spiel --> Spieler löschen
    // (im Spiel wird das dann von ExitGame übernommen, da die Spielerdaten evtl noch für
    // Statistiken usw. benötigt werden
    if(state != CS_GAME)
        players.clear();

    state = CS_STOPPED;

    if (IsHost())
        GAMESERVER.Stop();

    clientconfig.Clear();
    mapinfo.Clear();
    postMessages.clear();

    replayinfo.replay.StopRecording();

    // NFC-Queues aufräumen
    gcs.clear();

    socket.Close();

	// clear jump target
	skiptogf=0;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Startet ein Spiel oder Replay.
 *
 *  @param[in] random_init Initialwert des Zufallsgenerators.
 *
 *  @author OLiver
 */
void GameClient::StartGame(const unsigned int random_init)
{
    // Daten zurücksetzen
    randcheckinfo.Clear();

    // framesinfo vorinitialisieren
    // bei gespeicherten Spielen mit einem bestimmten GF natürlich beginnen!
    framesinfo.nr = (mapinfo.map_type == MAPTYPE_SAVEGAME) ? mapinfo.savegame.start_gf : 0;
    framesinfo.pause = true;

    // Je nach Geschwindigkeit GF-Länge einstellen
    framesinfo.gf_length = SPEED_GF_LENGTHS[ggs.game_speed];

    // Random-Generator initialisieren
    RANDOM.Init(random_init);

    // Spielwelt erzeugen
    gw = new GameWorld();
    gw->SetPlayers(&players);
    em = new EventManager();
    GameObject::SetPointers(gw, em, &players);
    for(unsigned i = 0; i < players.getCount(); ++i)
        dynamic_cast<GameClientPlayer*>(players.getElement(i))->SetGameWorldPointer(gw);

    if(ci)
        ci->CI_GameStarted(gw);

    Savegame* savegame = (mapinfo.map_type == MAPTYPE_SAVEGAME) ? &mapinfo.savegame : 0;

    if(savegame)
    {
        savegame->sgd.PrepareDeserialization(em);
        gw->Deserialize(&savegame->sgd);
        em->Deserialize(&savegame->sgd);
        for(unsigned i = 0; i < players.getCount(); ++i)
            GetPlayer(i)->Deserialize(&savegame->sgd);

        // TODO: schöner machen:
        // Die Fläche, die nur von einem Allierten des Spielers gesehen werden, müssen noch dem TerrainRenderer mitgeteilt werden
        // oder entsprechende Flächen müssen vorher bekannt gemacht werden
        // Die folgende Schleife aktualisiert einfach *alle* Punkt, ist also ziemlich ineffizient
        unsigned short height = gw->GetHeight();
        unsigned short width =  gw->GetWidth();
        for (unsigned short y = 0; y < height; ++y)
        {
            for (unsigned short x = 0; x < width; ++x)
            {
                gw->VisibilityChanged(x, y);
            }
        }
        // Visuelle Einstellungen ableiten
        GetVisualSettings();
    }
    else
    {
        /// Startbündnisse setzen
        for(unsigned i = 0; i < GetPlayerCount(); ++i)
            players[i].MakeStartPacts();

        gw->LoadMap(clientconfig.mapfilepath);

        /// Evtl. Goldvorkommen ändern
        unsigned char target = 0xFF; // löschen
        switch(GameClient::inst().GetGGS().getSelection(ADDON_CHANGE_GOLD_DEPOSITS))
        {
            case 0: target = 3; break; //in Gold   konvertieren bzw. nichts tun
            case 1: target = 0xFF; break; // löschen
            case 2: target = 2; break; //in Eisen  konvertieren
            case 3: target = 1; break; //in Kohle  konvertieren
            case 4: target = 0; break; //in Granit konvertieren
        }
        if (target != 3)
            gw->ConvertMineResourceTypes(3, target);
    }

    // Zeit setzen
    framesinfo.lasttime = VideoDriverWrapper::inst().GetTickCount();
    framesinfo.lastmsgtime = framesinfo.lasttime;

    if(!replay_mode)
    {
        WriteReplayHeader(random_init);

        char filename[256], time_str[80];
        unser_time_t temp = TIME.CurrentTime();
        TIME.FormatTime(time_str, "game_%Y-%m-%d_%H-%i-%s", &temp);
        sprintf(filename, "%s%s-%u.log", FILE_PATHS[47], time_str, rand() % 100);

        game_log = fopen(filename, "a");
    }

    // Daten nach dem Schreiben des Replays ggf wieder löschen
    if(mapinfo.zipdata)
    {
        delete [] mapinfo.zipdata;
        replayinfo.replay.map_data = 0;
        mapinfo.zipdata = NULL;
    }
}

///////////////////////////////////////////////////////////////////////////////
/*
 *
 *
 *  @author OLiver
 */
void GameClient::RealStart()
{
//  framesinfo.pause = false;
    framesinfo.pause = replay_mode;

	//framesinfo.pause = true;

    /// Wenn Replay, evtl erstes Command vom Start-Frame auslesen, was sonst ignoriert werden würde
    if(replay_mode)
        ExecuteGameFrame_Replay();

    GameManager::inst().ResetAverageFPS();
}

///////////////////////////////////////////////////////////////////////////////
/*
 *
 *
 *  @author OLiver
 */
void GameClient::ExitGame()
{
    // Spielwelt zerstören
    delete gw;
    delete em;
    gw = 0;
    em = 0;

    players.clear();

    GameObject::SetPointers(gw, em, &players);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Dead-Nachricht.
 *
 *  @author FloSoft
 */
void GameClient::OnNMSDeadMsg(unsigned int id)
{
    ServerLost();
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Ping-Nachricht.
 *
 *  @author FloSoft
 */
void GameClient::OnNMSPing(const GameMessage_Ping& msg)
{
    send_queue.push(new GameMessage_Pong(0xFF));
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Player-ID-Nachricht.
 *
 *  @author FloSoft
 */
void GameClient::OnNMSPlayerId(const GameMessage_Player_Id& msg)
{
    // haben wir eine ungültige ID erhalten? (aka Server-Voll)
    if(msg.playerid == 0xFFFFFFFF)
    {
        if(ci)
            ci->CI_Error(CE_SERVERFULL);

        Stop();
        return;
    }

    this->playerid = msg.playerid;

    // Server-Typ senden
    send_queue.push(new GameMessage_Server_Type(clientconfig.servertyp, GetWindowVersion()));
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Player-List-Nachricht.
 *
 *  @author FloSoft
 */
void GameClient::OnNMSPlayerList(const GameMessage_Player_List& msg)
{
    for(unsigned int i = 0; i < players.getCount(); ++i)
    {
        players[i].ps = msg.gpl[i].ps;
        players[i].name = msg.gpl[i].name;
        players[i].origin_name = msg.gpl[i].origin_name;
        players[i].is_host = msg.gpl[i].is_host;
        players[i].nation = msg.gpl[i].nation;
        players[i].team = msg.gpl[i].team;
        players[i].color = msg.gpl[i].color;
        players[i].ping = msg.gpl[i].ping;
        players[i].rating = msg.gpl[i].rating;
        players[i].ps = msg.gpl[i].ps;
        if(players[i].ps == PS_KI)
        {
            if(!strncmp(players[i].name.c_str(), "Computer", 7))
            {
                players[i].aiType = AI_JH;
                players[i].rating = 666;
            }
            else
            {
                players[i].aiType = AI_DUMMY;
                players[i].rating = 0;
            }
        }

        GamePlayerInfo* player = players.getElement(i);

        if(ci)
            ci->CI_PSChanged(i, player->ps);

        if(player->ps == PS_KI)
        {
            player->ready = true;
            if(ci)
                ci->CI_ReadyChanged(i, player->ready);
        }
    }

    if(ci)
        ci->CI_NextConnectState(CS_FINISHED);
}

///////////////////////////////////////////////////////////////////////////////
/// player joined
/// @param message  Nachricht, welche ausgeführt wird
inline void GameClient::OnNMSPlayerNew(const GameMessage_Player_New& msg)
{
    LOG.write("<<< NMS_PLAYER_NEW(%d)\n", msg.player );

    if(msg.player != 0xFF)
    {
        if(msg.player < players.getCount())
        {
            players[msg.player].name = msg.name;
            players[msg.player].ps = PS_OCCUPIED;
            players[msg.player].ping = 0;
            players[msg.player].rating = 1000;

            if(ci)
                ci->CI_NewPlayer(msg.player);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// player joined
/// @param message  Nachricht, welche ausgeführt wird
void GameClient::OnNMSPlayerPing(const GameMessage_Player_Ping& msg)
{
    if(msg.player != 0xFF)
    {
        if(msg.player < players.getCount())
        {
            players[msg.player].ping = msg.ping;

            if(ci)
                ci->CI_PingChanged(msg.player, msg.ping);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Player-Toggle-State-Nachricht.
 *
 *  @author FloSoft
 */
void GameClient::OnNMSPlayerToggleState(const GameMessage_Player_Toggle_State& msg)
{
    GameClientPlayer* player = players.getElement(msg.player);

    if(msg.player != 0xFF)
    {
        if(msg.player < players.getCount())
        {
            switch(player->ps)
            {
                case PS_FREE:
                {
                    player->ps = PS_KI;
                    player->aiType = AI_JH;
                    // Baby mit einem Namen Taufen ("Name (KI)")
                    char str[512];
                    sprintf(str, _("Computer %u"), unsigned(msg.player));
                    player->name = str;
                    player->name += _(" (AI)");
                    player->rating = 666;
                } break;
                case PS_KI:
                {
                    // Verschiedene KIs durchgehen
                    switch(player->aiType)
                    {
                        case AI_JH:
                            player->aiType = AI_DUMMY;
                            char str[512];
                            sprintf(str, _("Dummy %u"), unsigned(msg.player));
                            player->name = str;
                            player->rating = 0; // ;-)
                            break;
                        case AI_DUMMY:
                            if(mapinfo.map_type != MAPTYPE_SAVEGAME)
                                player->ps = PS_LOCKED;
                            else
                                player->ps = PS_FREE;
                            break;
                        default:
                            if(mapinfo.map_type != MAPTYPE_SAVEGAME)
                                player->ps = PS_LOCKED;
                            else
                                player->ps = PS_FREE;
                            break;
                    }
                    break;
                }
                case PS_LOCKED:
                {
                    // Im Savegame können auf geschlossene Slots keine Spieler
                    // gesetzt werden, der entsprechende Spieler existierte ja gar nicht auf
                    // der Karte!
                    if(mapinfo.map_type != MAPTYPE_SAVEGAME)
                        player->ps = PS_FREE;
                } break;
                default: break;
            }

            if(ci)
                ci->CI_PSChanged(msg.player, player->ps);
        }
        player->ready = (player->ps == PS_KI);

        if(ci)
            ci->CI_ReadyChanged(msg.player, player->ready);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// nation button gedrückt
/// @param message  Nachricht, welche ausgeführt wird
inline void GameClient::OnNMSPlayerToggleNation(const GameMessage_Player_Toggle_Nation& msg)
{
    if(msg.player != 0xFF)
    {
        if(msg.player < players.getCount())
        {
            players[msg.player].nation = msg.nation;

            if(ci)
                ci->CI_NationChanged(msg.player, msg.nation);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// team button gedrückt
/// @param message  Nachricht, welche ausgeführt wird
inline void GameClient::OnNMSPlayerToggleTeam(const GameMessage_Player_Toggle_Team& msg)
{
    if(msg.player != 0xFF)
    {
        if(msg.player < players.getCount())
        {
            players[msg.player].team = msg.team;

            if(ci)
                ci->CI_TeamChanged(msg.player, msg.team);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// color button gedrückt
/// @param message  Nachricht, welche ausgeführt wird
inline void GameClient::OnNMSPlayerToggleColor(const GameMessage_Player_Toggle_Color& msg)
{
    if(msg.player != 0xFF)
    {
        if(msg.player < players.getCount())
        {
            players[msg.player].color = msg.color;

            if(ci)
                ci->CI_ColorChanged(msg.player, msg.color);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Ready-state eines Spielers hat sich geändert.
 *
 *  @param[in] message Nachricht, welche ausgeführt wird
 *
 *  @author FloSoft
 */
inline void GameClient::OnNMSPlayerReady(const GameMessage_Player_Ready& msg)
{
    LOG.write("<<< NMS_PLAYER_READY(%d, %s)\n", msg.player, (msg.ready ? "true" : "false"));

    if(msg.player != 0xFF)
    {
        if(msg.player < players.getCount())
        {
            players[msg.player].ready = msg.ready;

            if(ci)
                ci->CI_ReadyChanged(msg.player, players[msg.player].ready);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// player gekickt
/// @param message  Nachricht, welche ausgeführt wird
inline void GameClient::OnNMSPlayerKicked(const GameMessage_Player_Kicked& msg)
{
    LOG.write("<<< NMS_PLAYER_KICKED(%d, %d, %d)\n", msg.player, msg.cause, msg.param);

    if(msg.player != 0xFF)
    {
        if(msg.player < players.getCount())
        {
            if(state == CS_GAME && GLOBALVARS.ingame)
            {
                // Im Spiel anzeigen, dass der Spieler das Spiel verlassen hat
                players[msg.player].ps = PS_KI;
            }
            else
                players[msg.player].clear();

            if(ci)
                ci->CI_PlayerLeft(msg.player);
        }
    }
}

inline void GameClient::OnNMSPlayerSwap(const GameMessage_Player_Swap& msg)
{
    LOG.write("<<< NMS_PLAYER_SWAP(%u, %u)\n", msg.player, msg.player2);

    players[msg.player].SwapPlayer(players[msg.player2]);

    // Evtl. sind wir betroffen?
    if(playerid == msg.player)
        playerid = msg.player2;
    else if(playerid == msg.player2)
        playerid = msg.player;


    if(ci)
        ci->CI_PlayersSwapped(msg.player, msg.player2);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Server-Typ-Nachricht.
 *
 *  @author FloSoft
 */
inline void GameClient::OnNMSServerTypeOK(const GameMessage_Server_TypeOK& msg)
{
    switch(msg.err_code)
    {
        case 0: // ok
            break;

        default:
        case 1:
        {
            if(ci)
                ci->CI_Error(CE_INVALIDSERVERTYPE);
            Stop();
            return;
        } break;

        case 2:
        {
            if(ci)
                ci->CI_Error(CE_WRONGVERSION);
            Stop();
            return;
        } break;
    }

    send_queue.push(new GameMessage_Server_Password(clientconfig.password));

    if(ci)
        ci->CI_NextConnectState(CS_QUERYPW);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Server-Passwort-Nachricht.
 *
 *  @author FloSoft
 */
void GameClient::OnNMSServerPassword(const GameMessage_Server_Password& msg)
{
    if(msg.password != "true")
    {
        if(ci)
            ci->CI_Error(CE_WRONGPW);

        Stop();
        return;
    }

    send_queue.push(new GameMessage_Player_Name(SETTINGS.lobby.name));
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Server-Name-Nachricht.
 *
 *  @author FloSoft
 */
void GameClient::OnNMSServerName(const GameMessage_Server_Name& msg)
{
    clientconfig.gamename = msg.name;

    if(ci)
        ci->CI_NextConnectState(CS_QUERYPLAYERLIST);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Server-Start-Nachricht
 *
 *  @author FloSoft
 *  @author OLiver
 */
inline void GameClient::OnNMSServerStart(const GameMessage_Server_Start& msg)
{
    // NWF-Länge bekommen wir vom Server
    framesinfo.nwf_length = msg.nwf_length;

    /// Beim Host muss das Spiel nicht nochmal gestartet werden, das hat der Server schon erledigt
    if(!IsHost())
        StartGame(msg.random_init);

    // Nothing-Command für ersten Network-Frame senden
    SendNothingNC(0);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Server-Chat-Nachricht.
 *
 *  @author FloSoft
 */
void GameClient::OnNMSServerChat(const GameMessage_Server_Chat& msg)
{
    if(msg.player != 0xFF)
    {
        if(state == CS_GAME)
            /// Mit im Replay aufzeichnen
            replayinfo.replay.AddChatCommand(framesinfo.nr, msg.player, msg.destination, msg.text);

        GameClientPlayer* player = GetPlayer(msg.player);

        // Besiegte dürfen nicht mehr heimlich mit Verbündeten oder Feinden reden
        if(player->isDefeated() && msg.destination != CD_ALL)
            return;
        // Entscheiden, ob ich ein Gegner oder Vebündeter bin vom Absender
        bool ally = GetLocalPlayer()->IsAlly(msg.player);

        // Chatziel unerscheiden und ggf. nicht senden
        if(!ally && msg.destination == CD_ALLIES)
            return;
        if(ally && msg.destination == CD_ENEMIES && msg.player != playerid)
            return;

        if(ci)
            ci->CI_Chat(msg.player, msg.destination, msg.text);

    }
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Server-Async-Nachricht.
 *
 *  @author FloSoft
 */
void GameClient::OnNMSServerAsync(const GameMessage_Server_Async& msg)
{
    // Liste mit Namen und Checksummen erzeugen
    std::stringstream checksum_list;
    checksum_list << 23;
    for(unsigned int i = 0; i < players.getCount(); ++i)
    {
        checksum_list << players.getElement(i)->name << ": " << msg.checksums.at(i);
        if(i != players.getCount() - 1)
            checksum_list << ", ";
    }

    // Fehler ausgeben (Konsole)!
    LOG.lprintf(_("The Game is not in sync. Checksums of some players don't match."));
    LOG.lprintf(checksum_list.str().c_str());
    LOG.lprintf("\n");

    // Messenger im Game
    if(ci && GLOBALVARS.ingame)
        ci->CI_Async(checksum_list.str());

    char filename[256], time_str[80];
    unser_time_t temp = TIME.CurrentTime();
    TIME.FormatTime(time_str, "async_%Y-%m-%d_%H-%i-%s", &temp);

//  sprintf(filename,"%s%s-%u.log",  GetFilePath(FILE_PATHS[47]).c_str(), time_str, rand()%100);

//  Random::inst().SaveLog(filename);

//  LOG.lprintf("Async log saved at \"%s\"\n",filename);

    sprintf(filename, "%s%s.sav", GetFilePath(FILE_PATHS[85]).c_str(), time_str);

    GameClient::inst().WriteSaveHeader(filename);

    // Pausieren

}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Server-Countdown-Nachricht.
 *
 *  @author FloSoft
 */
void GameClient::OnNMSServerCountdown(const GameMessage_Server_Countdown& msg)
{
    if(ci)
        ci->CI_Countdown(msg.countdown);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Server-Cancel-Countdown-Nachricht.
 *
 *  @author FloSoft
 */
void GameClient::OnNMSServerCancelCountdown(const GameMessage_Server_CancelCountdown& msg)
{
    if(ci)
        ci->CI_CancelCountdown();
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  verarbeitet die MapInfo-Nachricht, in der die gepackte GröÃŸe,
 *  die normale GröÃŸe und Teilanzahl der Karte übertragen wird.
 *
 *  @param message Nachricht, welche ausgeführt wird
 *
 *  @author FloSoft
 */
inline void GameClient::OnNMSMapInfo(const GameMessage_Map_Info& msg)
{
    // shortname
    clientconfig.mapfile = msg.map_name;
    // full path
    clientconfig.mapfilepath = GetFilePath(FILE_PATHS[48]) + clientconfig.mapfile;

    mapinfo.map_type = msg.mt;
    mapinfo.partcount = msg.partcount;
    mapinfo.ziplength = msg.ziplength;
    mapinfo.length = msg.normal_length;
    
    // lua script file path
    if (msg.script.length() > 0)
    {
        std::string lua_file = clientconfig.mapfilepath.substr(0, clientconfig.mapfilepath.length() - 3);
        lua_file.append("lua");

        FILE *lua_f = fopen(lua_file.c_str(), "wb");

        if ((lua_f == NULL) || (fwrite(msg.script.data(), 1, msg.script.length(), lua_f) != msg.script.length()))
        {
            LOG.lprintf("Fatal error: can't %s lua script to %s: %s\n", (lua_f == NULL) ? "open" : "write to", lua_file.c_str(), strerror(errno));

            Stop();
            return;
        }
        
        fclose(lua_f);
    }
    
    temp_ui = 0;
    temp_ul = 0;

    if(mapinfo.zipdata)
        delete[] mapinfo.zipdata;
    mapinfo.zipdata = new unsigned char[mapinfo.ziplength + 1];
}

///////////////////////////////////////////////////////////////////////////////
/// Kartendaten
/// @param message  Nachricht, welche ausgeführt wird
inline void GameClient::OnNMSMapData(const GameMessage_Map_Data& msg)
{
    LOG.write("<<< NMS_MAP_DATA(%u)\n", msg.GetNetLength());

    unsigned char* data = msg.map_data;

    memcpy(&mapinfo.zipdata[temp_ul], data, msg.GetNetLength());
    temp_ul += msg.GetNetLength();

    ++(temp_ui);
    if(temp_ui >= mapinfo.partcount)
    {
        FILE* map_f = fopen(clientconfig.mapfilepath.c_str(), "wb");

        if(!map_f)
        {
            LOG.lprintf("Fatal error: can't write map to %s: %s\n", clientconfig.mapfilepath.c_str(), strerror(errno));

            Stop();
            return;
        }

        char* map_data = new char[mapinfo.length + 1];

        unsigned int length = mapinfo.length;

        int err = BZ_OK;
        if( (err = BZ2_bzBuffToBuffDecompress(map_data, &length, (char*)mapinfo.zipdata, mapinfo.ziplength, 0, 0)) != BZ_OK)
        {
            LOG.lprintf("FATAL ERROR: BZ2_bzBuffToBuffDecompress failed with code %d\n", err);
            Stop();
            return;
        }
        if(fwrite(map_data, 1, mapinfo.length, map_f) != mapinfo.length)
            LOG.lprintf("ERROR: fwrite failed\n");

        mapinfo.checksum = CalcChecksumOfBuffer((unsigned char*)map_data, mapinfo.length);
        delete[] map_data;

        fclose(map_f);

        // Map-Typ unterscheiden
        switch(mapinfo.map_type)
        {
            case MAPTYPE_OLDMAP:
            {
                libsiedler2::ArchivInfo map;

                // Karteninformationen laden
                if(libsiedler2::loader::LoadMAP(clientconfig.mapfilepath.c_str(), &map, true) != 0)
                {
                    LOG.lprintf("GameClient::OnNMSMapData: ERROR: Map \"%s\", couldn't load header!\n", clientconfig.mapfilepath.c_str());
                    Stop();
                    return;
                }

                const libsiedler2::ArchivItem_Map_Header* header = &(dynamic_cast<const glArchivItem_Map*>(map.get(0))->getHeader());
                assert(header);

                players.clear();
                for(unsigned i = 0; i < header->getPlayer(); ++i)
                    players.push_back(GameClientPlayer(i));

                mapinfo.title = header->getName();

            } break;
            case MAPTYPE_SAVEGAME:
            {
                if(!mapinfo.savegame.Load(clientconfig.mapfilepath.c_str(), true, true))
                {
                    Stop();
                    return;
                }

                players.clear();
                for(unsigned i = 0; i < mapinfo.savegame.player_count; ++i)
                    players.push_back(GameClientPlayer(i));

                mapinfo.title = mapinfo.savegame.map_name;


            } break;
            case MAPTYPE_RTTRMAP:
                break;
            case MAPTYPE_RANDOMMAP:
                break;
        }

        send_queue.push(new GameMessage_Map_Checksum(mapinfo.checksum));

        LOG.write(">>>NMS_MAP_CHECKSUM(%u)\n", mapinfo.checksum);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// map-checksum
/// @param message  Nachricht, welche ausgeführt wird
inline void GameClient::OnNMSMapChecksumOK(const GameMessage_Map_ChecksumOK& msg)
{
    LOG.write("<<< NMS_MAP_CHECKSUM(%d)\n", msg.correct ? 1 : 0);

    if(msg.correct == false)
    {
        if(ci)
            ci->CI_Error(CE_WRONGMAP);

        Stop();
        return;
    }

}

///////////////////////////////////////////////////////////////////////////////
/// server typ
/// @param message  Nachricht, welche ausgeführt wird
void GameClient::OnNMSGGSChange(const GameMessage_GGSChange& msg)
{
    LOG.write("<<< NMS_GGS_CHANGE\n");

    ggs = msg.ggs;

    if(ci)
        ci->CI_GGSChanged(ggs);
}

///////////////////////////////////////////////////////////////////////////////
/// NFC Antwort vom Server
/// @param message  Nachricht, welche ausgeführt wird
void GameClient::OnNMSGameCommand(const GameMessage_GameCommand& msg)
{
    if(msg.player != 0xFF)
        // Nachricht in Queue einhängen
        players[msg.player].gc_queue.push_back(msg);
}

///////////////////////////////////////////////////////////////////////////////
/// Speed change message vom Server
/// @param message  Nachricht, welche ausgeführt wird
void GameClient::OnNMSServerSpeed(const GameMessage_Server_Speed& msg)
{
}

void GameClient::IncreaseSpeed()
{
    int gf_length = framesinfo.gf_length;
    if(gf_length > 10)
        gf_length -= 10;

#ifndef NDEBUG
    else if (gf_length == 10)
        gf_length = 1;
#endif

    else
        gf_length = 70;

    send_queue.push(new GameMessage_Server_Speed(gf_length));
}

void GameClient::IncreaseReplaySpeed()
{
	if (replay_mode)
	{
		if (framesinfo.gf_length > 10)
		{
			framesinfo.gf_length -= 10;
		} else if (framesinfo.gf_length == 10)
		{
			framesinfo.gf_length = 1;
		}
	}
}

void GameClient::DecreaseReplaySpeed()
{
	if (replay_mode)
	{
		if (framesinfo.gf_length == 1)
		{
			framesinfo.gf_length = 10;
		} else if (framesinfo.gf_length < 1000)
		{
			framesinfo.gf_length += 10;
		}
	}
}


///////////////////////////////////////////////////////////////////////////////
/// NFC Done vom Server
/// @param message  Nachricht, welche ausgeführt wird
void GameClient::OnNMSServerDone(const GameMessage_Server_NWFDone& msg)
{
    framesinfo.gf_length_new = msg.gf_length;

    framesinfo.nr_srv = msg.nr + framesinfo.nwf_length;

    if(msg.first)
    {
        state = CS_GAME; // zu gamestate wechseln
        RealStart();
    }

    //LOG.lprintf("framesinfo.nr(%d) == framesinfo.nr_srv(%d)\n", framesinfo.nr, framesinfo.nr_srv);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  NFC Pause-Nachricht von Server
 *
 *  @param[in] message Nachricht, welche ausgeführt wird
 *
 *  @author FloSoft
 */
void GameClient::OnNMSPause(const GameMessage_Pause& msg)
{
    //framesinfo.pause =  msg.paused;
	if(msg.paused)
		framesinfo.pause_gf = msg.nr;
	else
	{
	    framesinfo.pause =  false;
		framesinfo.pause_gf = 0;
	}

    framesinfo.lastmsgtime = VideoDriverWrapper::inst().GetTickCount();

    LOG.write("<<< NMS_NFC_PAUSE(%u)\n", framesinfo.pause_gf);

    if(msg.paused)
        ci->CI_GamePaused();
    else
        ci->CI_GameResumed();

}

///////////////////////////////////////////////////////////////////////////////
/**
 *  NFC GetAsyncLog von Server
 *
 *  @param[in] message Nachricht, welche ausgeführt wird
 *
 *  @author Maqs
 */
void GameClient::OnNMSGetAsyncLog(const GameMessage_GetAsyncLog& msg)
{
    // AsyncLog an den Server senden

    // stückeln...
    std::list<RandomEntry>* async_log = RANDOM.GetAsyncLog();

    std::list<RandomEntry> part;

    for (std::list<RandomEntry>::iterator it = async_log->begin(); it != async_log->end(); ++it)
    {
        part.push_back(*it);

        if (part.size() == 10)
        {
            send_queue.push(new GameMessage_SendAsyncLog(&part, false));
            part.clear();
        }
    }

    send_queue.push(new GameMessage_SendAsyncLog(&part, true));

    part.clear();
}

/// Findet heraus, ob ein Spieler laggt und setzt bei diesen Spieler den entsprechenden flag
bool GameClient::IsPlayerLagging()
{
    bool is_lagging = false;

    for(unsigned char i = 0; i < players.getCount(); ++i)
    {
        if(players[i].ps == PS_OCCUPIED || players[i].ps == PS_KI)
        {
            if(players[i].gc_queue.size() == 0)
            {
                players[i].is_lagging = true;
                is_lagging = true;
            }
            else
                players[i].is_lagging = false;
        }
    }

    return is_lagging;
}

/// Führt für alle Spieler einen Statistikschritt aus, wenn die Zeit es verlangt
void GameClient::StatisticStep()
{
    // Soll alle 750 GFs (30 Sekunden auf 'Schnell') aufgerufen werden
    if ((framesinfo.nr - 1) % 750 == 0)
    {
        for (unsigned int i = 0; i < players.getCount(); ++i)
        {
            players[i].StatisticStep();
        }

        // Check objective if there is one and there are at least two players
        if (ggs.game_objective != GlobalGameSettings::GO_NONE)
        {
            // check winning condition
            unsigned int max = 0, sum = 0, best = 0xFFFF, maxteam = 0, teampoints = 0, curteam = 0, bestteam = 0xFFFF;

            // Find out best player. Since at least 3/4 of the populated land is needed to win, we don't care about ties.
            for (unsigned int i = 0; i < players.getCount(); ++i)
            {
                if(ggs.lock_teams) //in games with locked team settings check for team victory
                {
                    curteam = 0;
                    teampoints = 0;
                    if(players[i].isDefeated())continue;
                    for(unsigned int j = 0; j < players.getCount(); ++j)
                    {
                        if(i != j && players[j].IsAlly(i) && !players[j].isDefeated())
                        {
                            curteam = curteam | (1 << j);
                            teampoints += players[j].GetStatisticCurrentValue(STAT_COUNTRY);
                        }
                    }
                    teampoints += players[i].GetStatisticCurrentValue(STAT_COUNTRY);
                    curteam = curteam | (1 << i);
                    if(teampoints > maxteam && teampoints - players[i].GetStatisticCurrentValue(STAT_COUNTRY) > 0)
                    {
                        maxteam = teampoints;
                        bestteam = curteam;
                    }
                }
                unsigned int v = players[i].GetStatisticCurrentValue(STAT_COUNTRY);
                if (v > max)
                {
                    max = v;
                    best = i;
                }

                sum += v;
            }

            switch (ggs.game_objective)
            {
                case GlobalGameSettings::GO_CONQUER3_4: // at least 3/4 of the land
                    if ((max * 4 >= sum * 3) && (best != 0xFFFF))
                    {
                        ggs.game_objective = GlobalGameSettings::GO_NONE;
                    }
                    if ((maxteam * 4 >= sum * 3) && (bestteam != 0xFFFF))
                    {
                        ggs.game_objective = GlobalGameSettings::GO_NONE;
                    }
                    break;

                case GlobalGameSettings::GO_TOTALDOMINATION:    // whole populated land
                    if ((max == sum) && (best != 0xFFFF))
                    {
                        ggs.game_objective = GlobalGameSettings::GO_NONE;
                    }
                    if ((maxteam == sum) && (bestteam != 0xFFFF))
                    {
                        ggs.game_objective = GlobalGameSettings::GO_NONE;
                    }
                    break;
                default:
                    break;
            }

            // We have a winner! Objective was changed to GO_NONE to avoid further checks.
            if (ggs.game_objective == GlobalGameSettings::GO_NONE)
            {
                if(maxteam <= best)
                {
                    gw->GetGameInterface()->GI_Winner(best);
                }
                else
                {
                    gw->GetGameInterface()->GI_TeamWinner(bestteam);
                }
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// testet ob ein Netwerkframe abgelaufen ist und führt dann ggf die Befehle aus
void GameClient::ExecuteGameFrame(const bool skipping)
{
    unsigned int currenttime = VideoDriverWrapper::inst().GetTickCount();
	if(!framesinfo.pause && framesinfo.pause_gf != 0 && framesinfo.nr == framesinfo.pause_gf)
	{
		framesinfo.pause_gf = 0;
		framesinfo.pause = true;
	}

    if(framesinfo.pause)
    {
        if(!replay_mode && ((currenttime - framesinfo.lastmsgtime) > 10000) )
        {
            //if(ci && GLOBALVARS.ingame)
            //  ci->CI_GamePaused();

            framesinfo.lastmsgtime = currenttime;
        }

        // pause machen ;)

        return;
    }

    // Wurde der nächsten Game-Frame zeitlich erreicht (bzw. wenn nur Frames übersprungen werden sollen,
    // brauchen wir nicht zu warten)?
    if(skipping || skiptogf > framesinfo.nr || (currenttime - framesinfo.lasttime) > framesinfo.gf_length)
    {
        //LOG.lprintf("%d = %d\n", framesinfo.nr / framesinfo.nwf_length, Random::inst().GetCurrentRandomValue());
        if(replay_mode)
        {

            // Diesen Zeitpunkt merken
            framesinfo.lasttime += framesinfo.gf_length;
            // Nächster Game-Frame erreicht
            ++framesinfo.nr;

            ExecuteGameFrame_Replay();

            // Frame-Time setzen zum Zeichnen, (immer auÃŸer bei Lags)
            framesinfo.frame_time = currenttime - framesinfo.lasttime;

            // Diesen Zeitpunkt merken
            framesinfo.lasttime = currenttime;
        }

        // Ist jetzt auch ein NWF dran?
        else if(framesinfo.nr % framesinfo.nwf_length == 0)
        {
            // entsprechenden NC für diesen NWF ausführen
            // Beim Replay geht das etwas anderes, da werden die NFCs aus der Datei gelesen

            // Schauen wir mal ob alles angekommen ist
            // Laggt einer oder nicht?
            if(!IsPlayerLagging())
            {
                // Kein Lag, normal weitermachen

                // Diesen Zeitpunkt merken
                framesinfo.lasttime = currenttime;
                // Nächster Game-Frame erreicht
                ++framesinfo.nr;

                ExecuteGameFrame_Game();

                // Frame-Time setzen zum Zeichnen, (immer auÃŸer bei Lags)
                framesinfo.frame_time = currenttime - framesinfo.lasttime;

            } // if(!is_lagging)

            if(framesinfo.gf_length_new != framesinfo.gf_length)
            {
                framesinfo.gf_length = framesinfo.gf_length_new;

                //int oldgfl = framesinfo.gf_length;
                int oldnwf = framesinfo.nwf_length;

                if(framesinfo.gf_length == 1)
                    framesinfo.nwf_length = 50;
                else
                    framesinfo.nwf_length = 250 / framesinfo.gf_length;

                framesinfo.nr_srv = framesinfo.nr_srv - oldnwf + framesinfo.nwf_length;

                LOG.lprintf("Client: %d/%d: Speed changed from %d to %d\n", framesinfo.nr_srv, framesinfo.nr, oldnwf, framesinfo.nwf_length);
                //LOG.lprintf("Client: GF-Length: %5d => %5d, NWF-Length: %5d => %5d, GF: %5d\n", oldgfl, framesinfo.gf_length, oldnwf, framesinfo.nwf_length, framesinfo.nr_srv);
            }
        } // if(framesinfo.nr % framesinfo.nwf_length == 0)
        else if (framesinfo.nr < framesinfo.nr_srv)
        {
            // Nähster GameFrame zwischen framesinfos

            // Diesen Zeitpunkt merken
            framesinfo.lasttime = currenttime;
            // Nächster Game-Frame erreicht
            ++framesinfo.nr;

            // Frame ausführen
            NextGF();

            // Frame-Time setzen zum Zeichnen, (immer auÃŸer bei Lags)
            framesinfo.frame_time = currenttime - framesinfo.lasttime;
        }

        // Auto-Speichern ggf.

        // Aktiviert?
        if(SETTINGS.interface.autosave_interval && !replay_mode)
        {
            // Alle .... GF
            if(framesinfo.nr % SETTINGS.interface.autosave_interval == 0)
            {
                std::string tmp = GetFilePath(FILE_PATHS[85]).c_str();

                if (this->mapinfo.title.length())
                {
                    tmp += this->mapinfo.title;
                    tmp += " (";
                    tmp += _("Auto-Save");
                    tmp += ").sav";
                }
                else
                {
                    tmp += _("Auto-Save");
                    tmp += ".sav";
                }

                WriteSaveHeader(tmp);
            }
        }

        // GF-Ende im Replay aktualisieren
        if(!replay_mode)
            replayinfo.replay.UpdateLastGF(framesinfo.nr);

    } // if(skipping || (currenttime - framesinfo.lasttime) > framesinfo.gf_length)
    else
    {
        // Frame-Time setzen zum Zeichnen, (immer auÃŸer bei Lags)
        framesinfo.frame_time = currenttime - framesinfo.lasttime;
    }
}

/// Führt notwendige Dinge für nächsten GF aus
void GameClient::NextGF()
{
    // Statistiken aktualisieren
    StatisticStep();
    //  EventManager Bescheid sagen
    em->NextGF();
    // Notfallprogramm durchlaufen lassen
    for(unsigned char i = 0; i < players.getCount(); ++i)
    {
        if(players[i].ps == PS_OCCUPIED || players[i].ps == PS_KI)
            // Auf Notfall testen (Wenige Bretter/Steine und keine Holzindustrie)
            players[i].TestForEmergencyProgramm();
        if(players[i].ps == PS_OCCUPIED || players[i].ps == PS_KI)
        {
            // Bündnisse auf Aktualität überprüfen
            players[i].TestPacts();
        }
    }
    
    gw->LUA_EventGF(framesinfo.nr);
}


void GameClient::ExecuteAllGCs(const GameMessage_GameCommand& gcs, unsigned char* player_switch_old_id, unsigned char* player_switch_new_id)
{
    for(unsigned char i = 0; i < gcs.gcs.size(); ++i)
    {
        // NC ausführen
        gcs.gcs[i]->Execute(*gw, players[gcs.player], gcs.player);
        //// Wenn ein Spieler gewechselt werden soll...
        if(gcs.gcs[i]->GetType() == gc::SWITCHPLAYER && player_switch_old_id && player_switch_new_id)
        {
            // ...müssen wir uns das merken
            *player_switch_old_id = gcs.player;
            *player_switch_new_id = dynamic_cast<gc::SwitchPlayer*>(gcs.gcs[i])->GetNewPlayerId();
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Sendet ein NC-Paket ohne Befehle.
 *
 *  @author FloSoft
 *  @author OLiver
 */
void GameClient::SendNothingNC(int checksum)
{
    if(checksum == -1)
        checksum = Random::inst().GetCurrentRandomValue();

    /*GameMessage nfc(NMS_NFC_COMMANDS, 5);
    *static_cast<int*>(nfc.m_pData) = checksum;*/

    send_queue.push(new GameMessage_GameCommand(playerid, checksum, std::vector<gc::GameCommand*>()));
}

void GameClient::WriteReplayHeader(const unsigned random_init)
{
    // Dateiname erzeugen
    char filename[256], time[80];
    unser_time_t temp = TIME.CurrentTime();
    TIME.FormatTime(time, "%Y-%m-%d_%H-%i-%s", &temp);

    sprintf(filename, "%s%s.rpl", GetFilePath(FILE_PATHS[51]).c_str(), time);

    // Headerinfos füllen

    // Timestamp der Aufzeichnung
    replayinfo.replay.save_time = temp;
    /// NWF-Länge
    replayinfo.replay.nwf_length = framesinfo.nwf_length;
    // Random-Init
    replayinfo.replay.random_init = random_init;
    // Spieleranzahl
    replayinfo.replay.player_count = players.getCount();

    // Spielerdaten
    delete [] replayinfo.replay.players;
    replayinfo.replay.players = new SavedFile::Player[players.getCount()];

    // Spielerdaten
    for(unsigned char i = 0; i < players.getCount(); ++i)
    {
        replayinfo.replay.players[i].ps = unsigned(players[i].ps);

        if(players[i].ps != PS_LOCKED)
        {
            replayinfo.replay.players[i].name = players[i].name;
            replayinfo.replay.players[i].nation = players[i].nation;
            replayinfo.replay.players[i].color = players[i].color;
            replayinfo.replay.players[i].team = players[i].team;
        }
    }

    // GGS-Daten
    replayinfo.replay.ggs = ggs;

    // Map

    replayinfo.replay.map_type = MapType(mapinfo.map_type);

    switch(replayinfo.replay.map_type)
    {
        default:
            break;

        case MAPTYPE_OLDMAP:
        {
            // GröÃŸe der entpackten Map
            replayinfo.replay.map_length  = mapinfo.length;
            // GröÃŸe der gepackten Map
            replayinfo.replay.map_zip_length = mapinfo.ziplength;
            // Gepackte Map
            replayinfo.replay.map_data = mapinfo.zipdata;
        } break;
        case MAPTYPE_SAVEGAME:
        {
            replayinfo.replay.savegame = &mapinfo.savegame;
        } break;
    }


    // Mapname
    replayinfo.replay.map_name = clientconfig.mapfile;

    // Datei speichern
    if(!replayinfo.replay.WriteHeader(filename))
        LOG.lprintf("GameClient::WriteReplayHeader: WARNING: File couldn't be opened. Don't use a replayinfo.replay.\n");
}

unsigned GameClient::StartReplay(const std::string& path, GameWorldViewer*& gwv)
{
    replayinfo.filename = path;
    replayinfo.replay.savegame = &mapinfo.savegame;

    if(!replayinfo.replay.LoadHeader(path, true))
        return false;

    // NWF-Länge
    framesinfo.nwf_length = replayinfo.replay.nwf_length;

    //players.resize(replayinfo.replay.players.getCount());

    // Spielerdaten
    for(unsigned char i = 0; i < replayinfo.replay.player_count; ++i)
    {
        players.push_back(GameClientPlayer(i));

        players[i].ps = PlayerState(replayinfo.replay.players[i].ps);

        if(players[i].ps != PS_LOCKED)
        {
            players[i].name = replayinfo.replay.players[i].name;
            players[i].nation = replayinfo.replay.players[i].nation;
            players[i].color = replayinfo.replay.players[i].color;
            players[i].team = Team(replayinfo.replay.players[i].team);
        }
    }

    // GGS-Daten
    ggs = replayinfo.replay.ggs;

    // Map-Type auslesen
    mapinfo.map_type = replayinfo.replay.map_type;

    switch(replayinfo.replay.map_type)
    {
        default:
            break;
        case MAPTYPE_OLDMAP:
        {
            // Mapdaten auslesen und entpacken
            unsigned char* real_data = new unsigned char[replayinfo.replay.map_length];

            int err;
            if( (err = BZ2_bzBuffToBuffDecompress((char*)real_data, &replayinfo.replay.map_length, (char*)replayinfo.replay.map_data, replayinfo.replay.map_zip_length, 0, 0)) != BZ_OK)
            {
                LOG.lprintf("FATAL ERROR: BZ2_bzBuffToBuffDecompress failed with code %d\n", err);
                Stop();
                return 5;
            }

            // Richtigen Pfad zur Map erstellen
            clientconfig.mapfile = replayinfo.replay.map_name;
            clientconfig.mapfilepath = GetFilePath(FILE_PATHS[48]) +  replayinfo.replay.map_name;

            // Und entpackte Mapdaten speichern
            BinaryFile map_f;
            if(!map_f.Open(clientconfig.mapfilepath.c_str(), OFM_WRITE))
            {
                LOG.lprintf("GameClient::StartReplay: ERROR: Couldn't open file \'%s\' for writing!\n", clientconfig.mapfilepath.c_str());
                return 6;
            }
            map_f.WriteRawData(real_data, replayinfo.replay.map_length);
            map_f.Close();

            delete[] real_data;
        } break;
        case MAPTYPE_SAVEGAME:
        {
        } break;
    }

    replay_mode = true;
    replayinfo.async = 0;
    replayinfo.end = false;

    StartGame(replayinfo.replay.random_init);

    replayinfo.replay.ReadGF(&replayinfo.next_gf);

	state = CS_GAME; // zu gamestate wechseln
    RealStart();

    gwv = gw;

    return 0;
}
//
//unsigned GameClient::GetGlobalAnimation(8,unsigned time_part_nominator, unsigned time_part_denominator,
//      unsigned divide_nominator, unsigned divide_denominator, unsigned offset)
//{
//  //return ((networkframe.nr * networkframe.length + networkframe.frame_time+offset) % time_part) / divide;
//  return ((networkframe.nr * networkframe.length + networkframe.frame_time+offset) % (networkframe.length*time_part_nominator/time_part_denominator)) / (networkframe.length*time_part_nominator/time_part_denominator);
//}

unsigned int GameClient::GetGlobalAnimation(const unsigned short max, const unsigned char factor_numerator, const unsigned char factor_denumerator, const unsigned int offset)
{
    // Unit for animations is 630ms (dividable by 2, 3, 5, 6, 7, 10, 15, ...)
    // But this also means: If framerate drops below approx. 15Hz, you won't see
    // every frame of an 8-part animation anymore.
    // An animation runs fully in (factor_numerator / factor_denumerator) multiples of 630ms
    const unsigned unit = 630/*ms*/ * factor_numerator / factor_denumerator;
    // Good approximation of current time in ms
    // (Accuracy of a possibly expensive VideoDriverWrapper::GetTicks() isn't needed here):
    const unsigned currenttime = framesinfo.lasttime + framesinfo.frame_time;
    return ((currenttime % unit) * max / unit + offset) % max;
}

unsigned GameClient::Interpolate(unsigned max_val, EventManager::EventPointer ev)
{
    assert( ev != NULL );
    return min<unsigned int>(((max_val * ((framesinfo.nr - ev->gf) * framesinfo.gf_length + framesinfo.frame_time)) / (ev->gf_length * framesinfo.gf_length)), max_val - 1);
}

int GameClient::Interpolate(int x1, int x2, EventManager::EventPointer ev)
{
    assert( ev != NULL );
    return (x1 + ( (x2 - x1) * ((int(framesinfo.nr) - int(ev->gf)) * int(framesinfo.gf_length) + int(framesinfo.frame_time))) / int(ev->gf_length * framesinfo.gf_length));
}

void GameClient::ServerLost()
{
    if(LOBBYCLIENT.LoggedIn()) // steht die Lobbyverbindung noch?
        LOBBYCLIENT.DeleteServer();

    if(ci)
        ci->CI_Error(CE_CONNECTIONLOST);


    if(state != CS_STOPPED)
        LOG.lprintf("client forced to stop\n");

    state = CS_STOPPED;

    socket.Close();
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  überspringt eine bestimmte Anzahl von Gameframes.
 *
 *  @param[in] dest_gf Zielgameframe
 *
 *  @author OLiver
 *  @author FloSoft
 */
void GameClient::SkipGF(unsigned int gf)
{
    if(gf <= framesinfo.nr)
        return;

    unsigned start_ticks = VideoDriverWrapper::inst().GetTickCount();

    // Spiel entpausieren
	if(replay_mode)
		SetReplayPause(false);
	if(!replay_mode)
	{
		//unpause before skipping
		if(GameServer::inst().IsPaused())
		{
			GameServer::inst().TogglePause();
			//return;
		}
		GameServer::inst().skiptogf=gf;
		skiptogf=gf;
		LOG.lprintf("jumping from gf %i to gf %i \n", framesinfo.nr, gf);
		return;
	}
	

    // GFs überspringen
	for(unsigned int i = framesinfo.nr; i < gf;++i)
    {
        if(i % 1000 == 0)
        {
            unsigned int water_percent;
            RoadsBuilding road;
            char nwf_string[256];

            road.mode = RM_DISABLED;
            road.point_x = 0;
            road.point_y = 0;
            road.start_x = 0;
            road.start_y = 0;

            // spiel aktualisieren
            gw->Draw(GetPlayerID(), &water_percent, false, 0, 0, road);

            // text oben noch hinschreiben
            snprintf(nwf_string, 255, _("current GF: %u - still fast forwarding: %d GFs left (%d %%)"), GetGFNumber(), gf - i, (i * 100 / gf) );			
            LargeFont->Draw(VideoDriverWrapper::inst().GetScreenWidth() / 2, VideoDriverWrapper::inst().GetScreenHeight() / 2, nwf_string, glArchivItem_Font::DF_CENTER, 0xFFFFFF00);

            VideoDriverWrapper::inst().SwapBuffers();
        }
        ExecuteGameFrame(true);
		//LOG.lprintf("jumping: now at gf %i\n", framesinfo.nr);		
    }
	
    // Spiel pausieren & text ausgabe wie lang das jetzt gedauert hat 
    unsigned ticks = VideoDriverWrapper::inst().GetTickCount() - start_ticks;
	char text[256];
	snprintf(text, sizeof(text), _("Jump finished (%.3f seconds)."), (double) ticks / 1000.0);
	ci->CI_Chat(playerid, CD_SYSTEM, text); 
	SetReplayPause(true);

}

void GameClient::SystemChat(std::string text)
{
    ci->CI_Chat(playerid, CD_SYSTEM, text);
}

unsigned GameClient::WriteSaveHeader(const std::string& filename)
{
    // Mond malen
    LOADER.GetImageN("resource", 33)->Draw(VideoDriverWrapper::inst().GetMouseX(), VideoDriverWrapper::inst().GetMouseY() - 40, 0, 0, 0, 0, 0, 0);
    VideoDriverWrapper::inst().SwapBuffers();

    Savegame save;

    // Timestamp der Aufzeichnung
    save.save_time = TIME.CurrentTime();
    // Mapname
    save.map_name = this->mapinfo.title;
    // Anzahl Spieler
    save.player_count = players.getCount();
    save.players = new SavedFile::Player[players.getCount()];

    // Spielerdaten
    for(unsigned char i = 0; i < players.getCount(); ++i)
    {
        save.players[i].ps = unsigned(players[i].ps);

        if(players[i].ps != PS_LOCKED)
        {
            save.players[i].name = players[i].name;
            save.players[i].nation = players[i].nation;
            save.players[i].color = players[i].color;
            save.players[i].team = players[i].team;
        }
    }

    // GGS-Daten
    save.ggs = ggs;

    save.start_gf = framesinfo.nr;



    // Spiel serialisieren
    save.sgd.MakeSnapshot(gw, em);

    // Und alles speichern
    if(!save.Save(filename))
        return 1;


    return 0;

}

void GameClient::GetVisualSettings()
{
    GameClientPlayer* player = GetLocalPlayer();
    //visual_settings.transport_order[0] = player->transport[GD_COINS];
    //visual_settings.transport_order[1] = player->transport[GD_SWORD];
    //visual_settings.transport_order[2] = player->transport[GD_BEER];
    //visual_settings.transport_order[3] = player->transport[GD_IRON];
    //visual_settings.transport_order[4] = player->transport[GD_GOLD];
    //visual_settings.transport_order[5] = player->transport[GD_IRONORE];
    //visual_settings.transport_order[6] = player->transport[GD_COAL];
    //visual_settings.transport_order[7] = player->transport[GD_BOARDS];
    //visual_settings.transport_order[8] = player->transport[GD_STONES];
    //visual_settings.transport_order[9] = player->transport[GD_WOOD];
    //visual_settings.transport_order[10] = player->transport[GD_WATER];
    //visual_settings.transport_order[11] = player->transport[GD_FISH];
    //visual_settings.transport_order[12] = player->transport[GD_HAMMER];
    //visual_settings.transport_order[13] = player->transport[GD_BOAT];

    visual_settings.transport_order[player->transport[GD_COINS]] = 0;
    visual_settings.transport_order[player->transport[GD_SWORD]] = 1;
    visual_settings.transport_order[player->transport[GD_BEER]] = 2;
    visual_settings.transport_order[player->transport[GD_IRON]] = 3;
    visual_settings.transport_order[player->transport[GD_GOLD]] = 4;
    visual_settings.transport_order[player->transport[GD_IRONORE]] = 5;
    visual_settings.transport_order[player->transport[GD_COAL]] = 6;
    visual_settings.transport_order[player->transport[GD_BOARDS]] = 7;
    visual_settings.transport_order[player->transport[GD_STONES]] = 8;
    visual_settings.transport_order[player->transport[GD_WOOD]] = 9;
    visual_settings.transport_order[player->transport[GD_WATER]] = 10;
    visual_settings.transport_order[player->transport[GD_FISH]] = 11;
    visual_settings.transport_order[player->transport[GD_HAMMER]] = 12;
    visual_settings.transport_order[player->transport[GD_BOAT]] = 13;



    visual_settings.distribution[0] = player->distribution[GD_FISH].percent_buildings[BLD_GRANITEMINE];
    visual_settings.distribution[1] = player->distribution[GD_FISH].percent_buildings[BLD_COALMINE];
    visual_settings.distribution[2] = player->distribution[GD_FISH].percent_buildings[BLD_IRONMINE];
    visual_settings.distribution[3] = player->distribution[GD_FISH].percent_buildings[BLD_GOLDMINE];

    visual_settings.distribution[4] = player->distribution[GD_GRAIN].percent_buildings[BLD_MILL];
    visual_settings.distribution[5] = player->distribution[GD_GRAIN].percent_buildings[BLD_PIGFARM];
    visual_settings.distribution[6] = player->distribution[GD_GRAIN].percent_buildings[BLD_DONKEYBREEDER];
    visual_settings.distribution[7] = player->distribution[GD_GRAIN].percent_buildings[BLD_BREWERY];
    visual_settings.distribution[8] = player->distribution[GD_GRAIN].percent_buildings[BLD_CHARBURNER];

    visual_settings.distribution[9] = player->distribution[GD_IRON].percent_buildings[BLD_ARMORY];
    visual_settings.distribution[10] = player->distribution[GD_IRON].percent_buildings[BLD_METALWORKS];

    visual_settings.distribution[11] = player->distribution[GD_COAL].percent_buildings[BLD_ARMORY];
    visual_settings.distribution[12] = player->distribution[GD_COAL].percent_buildings[BLD_IRONSMELTER];
    visual_settings.distribution[13] = player->distribution[GD_COAL].percent_buildings[BLD_MINT];

    visual_settings.distribution[14] = player->distribution[GD_WOOD].percent_buildings[BLD_SAWMILL];
    visual_settings.distribution[15] = player->distribution[GD_WOOD].percent_buildings[BLD_CHARBURNER];

    visual_settings.distribution[16] = player->distribution[GD_BOARDS].percent_buildings[BLD_HEADQUARTERS];
    visual_settings.distribution[17] = player->distribution[GD_BOARDS].percent_buildings[BLD_METALWORKS];
    visual_settings.distribution[18] = player->distribution[GD_BOARDS].percent_buildings[BLD_SHIPYARD];

    visual_settings.distribution[19] = player->distribution[GD_WATER].percent_buildings[BLD_BAKERY];
    visual_settings.distribution[20] = player->distribution[GD_WATER].percent_buildings[BLD_BREWERY];
    visual_settings.distribution[21] = player->distribution[GD_WATER].percent_buildings[BLD_PIGFARM];
    visual_settings.distribution[22] = player->distribution[GD_WATER].percent_buildings[BLD_DONKEYBREEDER];


    visual_settings.military_settings = player->military_settings;
    visual_settings.tools_settings = player->tools_settings;

    visual_settings.order_type = player->order_type;

    // Baureihenfolge füllen (0 ist das HQ!)
    for(unsigned char i = 0; i < 31; ++i)
        visual_settings.build_order[i] = player->build_order[i];
}


void GameClient::SetReplayPause(bool pause)
{
    if(replay_mode)
    {
        framesinfo.pause = pause;
        framesinfo.frame_time = 0;
    }
}


bool GameClient::AddGC(gc::GameCommand* gc)
{
    // Nicht in der Pause oder wenn er besiegt wurde
    if(framesinfo.pause || GetLocalPlayer()->isDefeated())
        return false;

    gcs.push_back(gc);
    return true;
}

/// Erzeugt einen KI-Player, der mit den Daten vom GameClient gefüttert werden muss (zusätzlich noch mit den GameServer)
AIBase* GameClient::CreateAIPlayer(const unsigned playerid)
{
    /*
    unsigned int level = AI::MEDIUM;

    switch(level)
    {
    case AI::EASY:
        {
            return new AIPlayer(playerid, gw,&players[playerid],&players,&ggs, AI::EASY);
        } break;
    default:
        {
            return new AIPlayerJH(playerid, gw,&players[playerid],&players,&ggs, (AI::Level)level);
        } break;
    }
    */

    switch (players[playerid].aiType)
    {
        case AI_DUMMY:
        {
            return new AIPlayer(playerid, gw, &players[playerid], &players, &ggs, AI::EASY);
        } break;
        case AI_JH:
        {
            return new AIPlayerJH(playerid, gw, &players[playerid], &players, &ggs, AI::EASY);
        } break;
        default:
        {
            return new AIPlayer(playerid, gw, &players[playerid], &players, &ggs, AI::EASY);
        } break;
    }

}

/// Wandelt eine GF-Angabe in eine Zeitangabe um (HH:MM:SS oder MM:SS wenn Stunden = 0)
std::string GameClient::FormatGFTime(const unsigned gf) const
{
    // In Sekunden umrechnen
    unsigned total_seconds = gf * framesinfo.gf_length / 1000;

    // Angaben rausfiltern
    unsigned hours = total_seconds / 3600;
    unsigned minutes =  total_seconds / 60;
    unsigned seconds = total_seconds % 60;

    char str[64];

    // ganze Stunden mit dabei? Dann entsprechend anderes format, ansonsten ignorieren wir die einfach
    if(hours)
        sprintf(str, "%02u:%02u:%02u", hours, minutes, seconds);
    else
        sprintf(str, "%02u:%02u", minutes, seconds);

    return std::string(str);
}


// Sendet eine Postnachricht an den Spieler
void GameClient::SendPostMessage(PostMsg* msg)
{
    if (postMessages.size() == MAX_POST_MESSAGES)
    {
        DeletePostMessage(*(--postMessages.end())); // muhahaha, geht das?
    }

    postMessages.push_front(msg);

    if(ci)
        ci->CI_NewPostMessage(postMessages.size());
}

// Entfernt eine Postnachricht aus der Liste und löscht sie
void GameClient::DeletePostMessage(PostMsg* msg)
{
    for(std::list<PostMsg*>::iterator it = postMessages.begin(); it != postMessages.end(); ++it)
    {
        if (msg == *it)
        {
            postMessages.erase(it);
            delete msg;

            if(ci)
                ci->CI_PostMessageDeleted(postMessages.size());
            break;
        }
    }
}

void GameClient::SendAIEvent(AIEvent::Base* ev, unsigned receiver)
{
    if (IsHost())
        GAMESERVER.SendAIEvent(ev, receiver);
    else
        delete ev;
}

/// Schreibt ggf. Pathfinding-Results in das Replay, falls erforderlich
void GameClient::AddPathfindingResult(const unsigned char dir, const unsigned* const length, const Point<MapCoord>* const next_harbor)
{
    // Sind wir im normalem Spiel?
    if(!replay_mode || (replay_mode && !replayinfo.replay.pathfinding_results))
    {
        // Dann hinzufügen
        replayinfo.replay.AddPathfindingResult(dir, length, next_harbor);
    }
}

/// Gibt zurück, ob Pathfinding-Results zur Verfügung stehen
bool GameClient::ArePathfindingResultsAvailable() const
{
    // PathfindingResults are buggy. TODO. Until fixed:
    return false;


    // Replaymodus?
    if(replay_mode || (!replay_mode && !replayinfo.replay.pathfinding_results))
    {
        // Unterstützt das Replay das auch (noch)?
        if(replayinfo.replay.pathfinding_results && !replayinfo.end)
            return true;
    }

    return false;
}

/// Gibt Pathfinding-Results zurück aus einem Replay
bool GameClient::ReadPathfindingResult(unsigned char* dir, unsigned* length, Point<MapCoord>* next_harbor)
{
    return replayinfo.replay.ReadPathfindingResult(dir, length, next_harbor);
}


/// Is tournament mode activated (0 if not)? Returns the durations of the tournament mode in gf otherwise
unsigned GameClient::GetTournamentModeDuration() const
{
    if(unsigned(ggs.game_objective) >= OBJECTIVES_COUNT)
        return TOURNAMENT_MODES_DURATION[ggs.game_objective - OBJECTIVES_COUNT] * 60 * 1000 / framesinfo.gf_length;
    else
        return 0;
}

#include <iostream>

void GameClient::LoadGGS()
{
    std::cout << "Loading settings...";
    ggs.LoadSettings();
    std::cout << "Done." << std::endl;
}
