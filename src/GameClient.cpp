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
#include "GameClient.h"

#include "GameManager.h"
#include "GameMessages.h"
#include "GameSavegame.h"
#include "GlobalVars.h"
#include "SocketSet.h"
#include "Loader.h"
#include "Settings.h"
#include "drivers/VideoDriverWrapper.h"
#include "Random.h"
#include "GameServer.h"
#include "EventManager.h"
#include "GameObject.h"
#include "GlobalGameSettings.h"
#include "lua/LuaInterfaceGame.h"
#include "gameData/GameConsts.h"
#include "PostMsg.h"
#include "SerializedGameData.h"
#include "LobbyClient.h"
#include "files.h"
#include "fileFuncs.h"
#include "ClientInterface.h"
#include "GameInterface.h"
#include "gameTypes/RoadBuildState.h"
#include "ai/AIPlayer.h"
#include "ai/AIPlayerJH.h"
#include "helpers/Deleter.h"
#include "ogl/glArchivItem_Map.h"
#include "ogl/glArchivItem_Font.h"
#include "world/GameWorldView.h"
#include "world/GameWorld.h"
#include "libsiedler2/src/prototypen.h"
#include "libsiedler2/src/ArchivItem_Map_Header.h"
#include <boost/smart_ptr/scoped_array.hpp>
#include <boost/interprocess/smart_ptr/unique_ptr.hpp>
#include <boost/filesystem.hpp>
#include <cerrno>
#include <iostream>
#include <fstream>

// Include last!
#include "DebugNew.h" // IWYU pragma: keep

class GameWorldViewer;

///////////////////////////////////////////////////////////////////////////////
/*
 *
 *
 *  @author
 */
void GameClient::ClientConfig::Clear()
{
    server.clear();
    gameName.clear();
    password.clear();
    port = 0;
    isHost = false;
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
    replay = Replay();
    async = 0;
    end = false;
    next_gf = 0;
    filename.clear();
    all_visible = false;
}

///////////////////////////////////////////////////////////////////////////////
/*
 *
 *
 *  @author FloSoft
 */
GameClient::GameClient()
    : skiptogf(0), gw(NULL), em(NULL), playerId_(0), recv_queue(&GameMessage::create_game), send_queue(&GameMessage::create_game), state(CS_STOPPED),
      ci(NULL), human_ai(NULL), replay_mode(false), game_log(NULL)
{
    clientconfig.Clear();
    framesinfo.Clear();
    randcheckinfo.Clear();
    postMessages.clear();
}

///////////////////////////////////////////////////////////////////////////////
/*
 *
 *
 *  @author FloSoft
 */
GameClient::~GameClient()
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
bool GameClient::Connect(const std::string& server, const std::string& password, ServerType servertyp, unsigned short port, bool host, bool use_ipv6)
{
    Stop();
    ggs.LoadSettings();

    // Name und Password kopieren
    clientconfig.server = server;
    clientconfig.password = password;

    clientconfig.servertyp = servertyp;
    clientconfig.port = port;
    clientconfig.isHost = host;

    // Verbinden
    if(!socket.Connect(server, port, use_ipv6, (Socket::PROXY_TYPE)SETTINGS.proxy.typ, SETTINGS.proxy.proxy, SETTINGS.proxy.port)) //-V807
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
        if(!recv_queue.recv(socket))
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
    send_queue.send(socket, 10);

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
    {
        if(human_ai)
            deletePtr(human_ai);
        players.clear();
    }

    state = CS_STOPPED;

    if (IsHost())
        GAMESERVER.Stop();

    framesinfo.Clear();
    clientconfig.Clear();
    mapinfo.Clear();
    for(std::list<PostMsg*>::iterator it = postMessages.begin(); it != postMessages.end(); ++it)
        delete *it;
    postMessages.clear();

    replayinfo.replay.StopRecording();

    // NFC-Queues aufräumen
    gameCommands_.clear();

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
    framesinfo.gf_nr = (mapinfo.type == MAPTYPE_SAVEGAME) ? mapinfo.savegame->start_gf : 0;
    framesinfo.isPaused = true;

    // Je nach Geschwindigkeit GF-Länge einstellen
    framesinfo.gf_length = SPEED_GF_LENGTHS[ggs.game_speed];
    framesinfo.gfLengthReq = framesinfo.gf_length;

    // Random-Generator initialisieren
    RANDOM.Init(random_init);

    // Spielwelt erzeugen
    gw = new GameWorld();
    gw->SetPlayers(&players);
    em = new EventManager();
    GameObject::SetPointers(gw, em);
    for(unsigned i = 0; i < players.getCount(); ++i)
        players[i].SetGameWorldPointer(gw);

    if(ci)
        ci->CI_GameStarted(gw);

    if(mapinfo.savegame)
    {
        mapinfo.savegame->sgd.ReadSnapshot(*gw, *em);

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
                gw->VisibilityChanged(MapPoint(x, y));
            }
        }
        // Visuelle Einstellungen ableiten
        ResetVisualSettings();
    }
    else
    {
        RTTR_Assert(mapinfo.type != MAPTYPE_SAVEGAME);
        /// Startbündnisse setzen
        for(unsigned i = 0; i < GetPlayerCount(); ++i)
            players[i].MakeStartPacts();

        gw->LoadMap(mapinfo.filepath, mapinfo.luaFilepath);

        /// Evtl. Goldvorkommen ändern
        unsigned char target = 0xFF; // löschen
        switch(GAMECLIENT.GetGGS().getSelection(AddonId::CHANGE_GOLD_DEPOSITS))
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
    framesinfo.lastTime = VIDEODRIVER.GetTickCount();

    if(!replay_mode)
    {
        WriteReplayHeader(random_init);
        std::stringstream fileName;
        fileName << FILE_PATHS[47] << TIME.FormatTime("game_%Y-%m-%d_%H-%i-%s")
                 << "-" << (rand() % 100) << ".log";

        game_log = fopen(fileName.str().c_str(), "a");
    }

    // Daten nach dem Schreiben des Replays ggf wieder löschen
    mapinfo.mapData.Clear();
}

///////////////////////////////////////////////////////////////////////////////
/*
 *
 *
 *  @author OLiver
 */
void GameClient::RealStart()
{
    RTTR_Assert(state == CS_LOADING);
    state = CS_GAME; // zu gamestate wechseln

    framesinfo.isPaused = replay_mode;

    // Send empty GC for first NWF
    if(!replay_mode)
        SendNothingNC(0);

    GAMEMANAGER.ResetAverageFPS();

    if(gw->HasLua())
        gw->GetLua().EventStart(!mapinfo.savegame);
}

///////////////////////////////////////////////////////////////////////////////
/*
 *
 *
 *  @author OLiver
 */
void GameClient::ExitGame()
{
    GameObject::SetPointers(NULL, NULL);
    // Spielwelt zerstören
    deletePtr(human_ai);
    deletePtr(gw);
    deletePtr(em);
    deletePtr(human_ai);
    players.clear();
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Ping-Nachricht.
 *
 *  @author FloSoft
 */
void GameClient::OnGameMessage(const GameMessage_Ping&  /*msg*/)
{
    send_queue.push(new GameMessage_Pong(0xFF));
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Player-ID-Nachricht.
 *
 *  @author FloSoft
 */
void GameClient::OnGameMessage(const GameMessage_Player_Id& msg)
{
    // haben wir eine ungültige ID erhalten? (aka Server-Voll)
    if(msg.playerid == 0xFFFFFFFF)
    {
        if(ci)
            ci->CI_Error(CE_SERVERFULL);

        Stop();
        return;
    }

    this->playerId_ = msg.playerid;

    // Server-Typ senden
    send_queue.push(new GameMessage_Server_Type(clientconfig.servertyp, GetWindowVersion()));
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Player-List-Nachricht.
 *
 *  @author FloSoft
 */
void GameClient::OnGameMessage(const GameMessage_Player_List& msg)
{
    for(unsigned int i = 0; i < players.getCount(); ++i)
    {
        GameClientPlayer& player = players[i];
        const GamePlayerInfo& msgPlayer = msg.gpl[i];
        player.ps = msgPlayer.ps;
        player.name = msgPlayer.name;
        player.origin_name = msgPlayer.origin_name;
        player.is_host = msgPlayer.is_host;
        player.nation = msgPlayer.nation;
        player.team = msgPlayer.team;
        player.color = msgPlayer.color;
        player.ping = msgPlayer.ping;
        player.rating = msgPlayer.rating;
        player.ps = msgPlayer.ps;
        player.aiInfo = msgPlayer.aiInfo;
        player.rating = msgPlayer.rating;

        if(ci)
            ci->CI_PSChanged(i, player.ps);

        if(player.ps == PS_KI)
        {
            player.ready = true;
            if(ci)
                ci->CI_ReadyChanged(i, player.ready);
        }
    }

    if(ci)
        ci->CI_NextConnectState(CS_FINISHED);
    state = CS_CONFIG;
}

///////////////////////////////////////////////////////////////////////////////
/// player joined
/// @param message  Nachricht, welche ausgeführt wird
inline void GameClient::OnGameMessage(const GameMessage_Player_New& msg)
{
    LOG.write("<<< NMS_PLAYER_NEW(%d)\n", msg.player );

    if(msg.player >= GetPlayerCount())
        return;

    players[msg.player].name = msg.name;
    players[msg.player].ps = PS_OCCUPIED;
    players[msg.player].ping = 0;
    players[msg.player].rating = 1000;

    if(ci)
        ci->CI_NewPlayer(msg.player);
}

///////////////////////////////////////////////////////////////////////////////
/// player joined
/// @param message  Nachricht, welche ausgeführt wird
void GameClient::OnGameMessage(const GameMessage_Player_Ping& msg)
{
    if(msg.player >= GetPlayerCount())
        return;

    players[msg.player].ping = msg.ping;

    if(ci)
        ci->CI_PingChanged(msg.player, msg.ping);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Player-Toggle-State-Nachricht.
 *
 *  @author FloSoft
 */
void GameClient::OnGameMessage(const GameMessage_Player_Set_State& msg)
{
    if(msg.player >= players.getCount())
        return;

    GameClientPlayer* player = players.getElement(msg.player);
    player->ps = msg.ps;
    player->aiInfo = msg.aiInfo;

    // Baby mit einem Namen Taufen ("Name (KI)")
    if (player->aiInfo.type == AI::DEFAULT)
    {
        char str[512];
        sprintf(str, _("Computer %u"), unsigned(msg.player));
        player->name = str;
        player->name += _(" (AI)");
        switch (player->aiInfo.level)
        {
        case AI::EASY:
            player->name += _(" (easy)");
            player->rating = 42;
            break;
        case AI::MEDIUM:
            player->name += _(" (medium)");
            player->rating = 666;
            break;
        case AI::HARD:
            player->name += _(" (hard)");
            player->rating = 1337;
            break;
        }
    }
    else if (player->aiInfo.type == AI::DUMMY)
    {
        char str[512];
        sprintf(str, _("Dummy %u"), unsigned(msg.player));
        player->name = str;
        player->rating = 0; // ;-)
    }

    player->ready = (player->ps == PS_KI);
    if(ci)
    {
        ci->CI_PSChanged(msg.player, player->ps);
        ci->CI_ReadyChanged(msg.player, player->ready);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// nation button gedrückt
/// @param message  Nachricht, welche ausgeführt wird
inline void GameClient::OnGameMessage(const GameMessage_Player_Set_Nation& msg)
{
    if(msg.player >= GetPlayerCount())
        return;

    players[msg.player].nation = msg.nation;

    if(ci)
        ci->CI_NationChanged(msg.player, msg.nation);
}

///////////////////////////////////////////////////////////////////////////////
/// team button gedrückt
/// @param message  Nachricht, welche ausgeführt wird
inline void GameClient::OnGameMessage(const GameMessage_Player_Set_Team& msg)
{
    if(msg.player >= GetPlayerCount())
        return;

    players[msg.player].team = msg.team;

    if(ci)
        ci->CI_TeamChanged(msg.player, msg.team);
}

///////////////////////////////////////////////////////////////////////////////
/// color button gedrückt
/// @param message  Nachricht, welche ausgeführt wird
inline void GameClient::OnGameMessage(const GameMessage_Player_Set_Color& msg)
{
    if(msg.player >= GetPlayerCount())
        return;

    players[msg.player].color = msg.color;

    if(ci)
        ci->CI_ColorChanged(msg.player, msg.color);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Ready-state eines Spielers hat sich geändert.
 *
 *  @param[in] message Nachricht, welche ausgeführt wird
 *
 *  @author FloSoft
 */
inline void GameClient::OnGameMessage(const GameMessage_Player_Ready& msg)
{
    LOG.write("<<< NMS_PLAYER_READY(%d, %s)\n", msg.player, (msg.ready ? "true" : "false"));

    if(msg.player >= GetPlayerCount())
        return;

    players[msg.player].ready = msg.ready;

    if(ci)
        ci->CI_ReadyChanged(msg.player, players[msg.player].ready);
}

///////////////////////////////////////////////////////////////////////////////
/// player gekickt
/// @param message  Nachricht, welche ausgeführt wird
inline void GameClient::OnGameMessage(const GameMessage_Player_Kicked& msg)
{
    LOG.write("<<< NMS_PLAYER_KICKED(%d, %d, %d)\n", msg.player, msg.cause, msg.param);

    if(msg.player >= GetPlayerCount())
        return;

    if(state == CS_GAME)
    {
        // Im Spiel anzeigen, dass der Spieler das Spiel verlassen hat
        players[msg.player].ps = PS_KI;
    }
    else
        players[msg.player].clear();

    if(ci)
        ci->CI_PlayerLeft(msg.player);
}

inline void GameClient::OnGameMessage(const GameMessage_Player_Swap& msg)
{
    LOG.write("<<< NMS_PLAYER_SWAP(%u, %u)\n", msg.player, msg.player2);

    if(msg.player >= GetPlayerCount() || msg.player2 >= GetPlayerCount())
        return;

    if(state == CS_GAME)
    {
        ChangePlayerIngame(msg.player, msg.player2);
    }else
    {
        // We are in preparation steps -> switch player info
        players[msg.player].SwapInfo(players[msg.player2]);

        // Evtl. sind wir betroffen?
        if(playerId_ == msg.player)
            playerId_ = msg.player2;
        else if(playerId_ == msg.player2)
            playerId_ = msg.player;

        if(ci)
            ci->CI_PlayersSwapped(msg.player, msg.player2);
    }
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Server-Typ-Nachricht.
 *
 *  @author FloSoft
 */
inline void GameClient::OnGameMessage(const GameMessage_Server_TypeOK& msg)
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
void GameClient::OnGameMessage(const GameMessage_Server_Password& msg)
{
    if(msg.password != "true")
    {
        if(ci)
            ci->CI_Error(CE_WRONGPW);

        Stop();
        return;
    }

    send_queue.push(new GameMessage_Player_Name(SETTINGS.lobby.name));

    if(ci)
        ci->CI_NextConnectState(CS_QUERYMAPNAME);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Server-Name-Nachricht.
 *
 *  @author FloSoft
 */
void GameClient::OnGameMessage(const GameMessage_Server_Name& msg)
{
    clientconfig.gameName = msg.name;

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
inline void GameClient::OnGameMessage(const GameMessage_Server_Start& msg)
{
    // NWF-Länge bekommen wir vom Server
    framesinfo.nwf_length = msg.nwf_length;
    state = CS_LOADING;

    /// Beim Host muss das Spiel nicht nochmal gestartet werden, das hat der Server schon erledigt
    if(IsHost())
        return;

    try
    {
        StartGame(msg.random_init);
    } catch(SerializedGameData::Error& error)
    {
        LOG.lprintf("Error when loading game: %s\n", error.what());
        GAMEMANAGER.ShowMenu();
        GAMECLIENT.ExitGame();
    }
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Server-Chat-Nachricht.
 *
 *  @author FloSoft
 */
void GameClient::OnGameMessage(const GameMessage_Server_Chat& msg)
{
    if(msg.player >= GetPlayerCount())
        return;

    if(state == CS_GAME)
        /// Mit im Replay aufzeichnen
        replayinfo.replay.AddChatCommand(framesinfo.gf_nr, msg.player, msg.destination, msg.text);

    GameClientPlayer& player = GetPlayer(msg.player);

    // Besiegte dürfen nicht mehr heimlich mit Verbüdeten oder Feinden reden
    if(player.isDefeated() && msg.destination != CD_ALL)
        return;
    // Entscheiden, ob ich ein Gegner oder Vebündeter bin vom Absender
    bool ally = GetLocalPlayer().IsAlly(msg.player);

    // Chatziel unerscheiden und ggf. nicht senden
    if(!ally && msg.destination == CD_ALLIES)
        return;
    if(ally && msg.destination == CD_ENEMIES && msg.player != playerId_)
        return;

    if(ci)
        ci->CI_Chat(msg.player, msg.destination, msg.text);
}

void GameClient::OnGameMessage(const GameMessage_System_Chat& msg)
{
    SystemChat(msg.text, msg.player);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Server-Async-Nachricht.
 *
 *  @author FloSoft
 */
void GameClient::OnGameMessage(const GameMessage_Server_Async& msg)
{
    // Liste mit Namen und Checksummen erzeugen
    std::stringstream checksum_list;
    for(unsigned int i = 0; i < msg.checksums.size(); ++i)
    {
        checksum_list << players.getElement(i)->name << ": " << msg.checksums[i];
        if(i + 1 < msg.checksums.size())
            checksum_list << ", ";
    }

    // Fehler ausgeben (Konsole)!
    LOG.lprintf(_("The Game is not in sync. Checksums of some players don't match."));
    LOG.lprintf(checksum_list.str().c_str());
    LOG.lprintf("\n");

    // Messenger im Game
    if(ci && state == CS_GAME)
        ci->CI_Async(checksum_list.str());

    std::string timeStr = TIME.FormatTime("async_%Y-%m-%d_%H-%i-%s");
    std::string filePathSave = GetFilePath(FILE_PATHS[85]) + timeStr + ".sav";
    std::string filePathLog = GetFilePath(FILE_PATHS[47]) + timeStr + "Player.log";
    RANDOM.SaveLog(filePathLog);
    GAMECLIENT.SaveToFile(filePathSave);
    LOG.lprintf("Async log saved at \"%s\", game saved at \"%s\"\n", filePathLog.c_str(), filePathSave.c_str());
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Server-Countdown-Nachricht.
 *
 *  @author FloSoft
 */
void GameClient::OnGameMessage(const GameMessage_Server_Countdown& msg)
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
void GameClient::OnGameMessage(const GameMessage_Server_CancelCountdown&  /*msg*/)
{
    if(ci)
        ci->CI_CancelCountdown();
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  verarbeitet die MapInfo-Nachricht, in der die gepackte Größe,
 *  die normale Größe und Teilanzahl der Karte übertragen wird.
 *
 *  @param message Nachricht, welche ausgeführt wird
 *
 *  @author FloSoft
 */
inline void GameClient::OnGameMessage(const GameMessage_Map_Info& msg)
{
    // full path
    mapinfo.filepath = GetFilePath(FILE_PATHS[48]) + msg.map_name;
    mapinfo.type = msg.mt;
    mapinfo.mapData.data.resize(msg.mapCompressedLen);
    mapinfo.mapData.length = msg.mapLen;
    mapinfo.luaData.data.resize(msg.luaCompressedLen);
    mapinfo.luaData.length = msg.luaLen;

    // lua script file path
    if (msg.luaLen > 0)
        mapinfo.luaFilepath = mapinfo.filepath.substr(0, mapinfo.filepath.length() - 3) + "lua";
    else
        mapinfo.luaFilepath.clear();
}

///////////////////////////////////////////////////////////////////////////////
/// Kartendaten
/// @param message  Nachricht, welche ausgeführt wird
inline void GameClient::OnGameMessage(const GameMessage_Map_Data& msg)
{
    LOG.write("<<< NMS_MAP_DATA(%u)\n", msg.data.size());
    if(msg.isMapData)
        std::copy(msg.data.begin(), msg.data.end(), mapinfo.mapData.data.begin() + msg.offset);
    else
        std::copy(msg.data.begin(), msg.data.end(), mapinfo.luaData.data.begin() + msg.offset);

    const unsigned curSize = msg.offset + msg.data.size();
    bool isCompleted;
    if(msg.isMapData)
        isCompleted = mapinfo.luaFilepath.empty() && curSize == mapinfo.mapData.data.size();
    else
        isCompleted = curSize == mapinfo.luaData.data.size();

    if(isCompleted)
    {
        if(!mapinfo.mapData.DecompressToFile(mapinfo.filepath, &mapinfo.mapChecksum))
        {
            if(ci)
                ci->CI_Error(CE_WRONGMAP);
            Stop();
            return;
        }
        if(!mapinfo.luaFilepath.empty() && !mapinfo.luaData.DecompressToFile(mapinfo.luaFilepath, &mapinfo.luaChecksum))
        {
            if(ci)
                ci->CI_Error(CE_WRONGMAP);
            Stop();
            return;
        }
        RTTR_Assert(!mapinfo.luaFilepath.empty() || mapinfo.luaChecksum == 0);

        // Map-Typ unterscheiden
        switch(mapinfo.type)
        {
            case MAPTYPE_OLDMAP:
            {
                libsiedler2::ArchivInfo map;

                // Karteninformationen laden
                if(libsiedler2::loader::LoadMAP(mapinfo.filepath, map, true) != 0)
                {
                    LOG.lprintf("GameClient::OnMapData: ERROR: Map \"%s\", couldn't load header!\n", mapinfo.filepath.c_str());
                    if(ci)
                        ci->CI_Error(CE_WRONGMAP);
                    Stop();
                    return;
                }

                const libsiedler2::ArchivItem_Map_Header* header = &(dynamic_cast<const glArchivItem_Map*>(map.get(0))->getHeader());
                RTTR_Assert(header);

                players.clear();
                for(unsigned i = 0; i < header->getPlayer(); ++i)
                    players.push_back(GameClientPlayer(i));

                mapinfo.title = cvStringToUTF8(header->getName());

            } break;
            case MAPTYPE_SAVEGAME:
            {
                mapinfo.savegame.reset(new Savegame);
                if(!mapinfo.savegame->Load(mapinfo.filepath, true, true))
                {
                    if(ci)
                        ci->CI_Error(CE_WRONGMAP);
                    Stop();
                    return;
                }

                players.clear();
                for(unsigned i = 0; i < mapinfo.savegame->GetPlayerCount(); ++i)
                    players.push_back(GameClientPlayer(i));

                mapinfo.title = mapinfo.savegame->mapName;


            } break;
        }

        if(playerId_ >= GetPlayerCount())
        {
            if(ci)
                ci->CI_Error(CE_WRONGMAP);
            Stop();
            return;
        }
        send_queue.push(new GameMessage_Map_Checksum(mapinfo.mapChecksum, mapinfo.luaChecksum));

        LOG.write(">>>NMS_MAP_CHECKSUM(%u)\n", mapinfo.mapChecksum);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// map-checksum
/// @param message  Nachricht, welche ausgeführt wird
inline void GameClient::OnGameMessage(const GameMessage_Map_ChecksumOK& msg)
{
    LOG.write("<<< NMS_MAP_CHECKSUM(%d)\n", msg.correct ? 1 : 0);

    if(!msg.correct)
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
void GameClient::OnGameMessage(const GameMessage_GGSChange& msg)
{
    LOG.write("<<< NMS_GGS_CHANGE\n");

    ggs = msg.ggs;

    if(ci)
        ci->CI_GGSChanged(ggs);
}

void GameClient::OnGameMessage(const GameMessage_RemoveLua& msg)
{
    RTTR_Assert(state == CS_CONNECT || state == CS_CONFIG);
    mapinfo.luaFilepath.clear();
    mapinfo.luaData.Clear();
    mapinfo.luaChecksum = 0;
}

///////////////////////////////////////////////////////////////////////////////
/// NFC Antwort vom Server
/// @param message  Nachricht, welche ausgeführt wird
void GameClient::OnGameMessage(const GameMessage_GameCommand& msg)
{
    if(msg.player >= GetPlayerCount())
        return;
    LOG.write("CLIENT <<< GC %u\n", msg.player);
    // Nachricht in Queue einhängen
    players[msg.player].gc_queue.push(msg);
    RTTR_Assert(msg.player != playerId_ || players[msg.player].gc_queue.size() == 1);
}

void GameClient::IncreaseSpeed()
{
    if(framesinfo.gfLengthReq > 10)
        framesinfo.gfLengthReq -= 10;

#ifndef NDEBUG
    else if (framesinfo.gfLengthReq == 10)
        framesinfo.gfLengthReq = 1;
#endif

    else
        framesinfo.gfLengthReq = 70;

    send_queue.push(new GameMessage_Server_Speed(framesinfo.gfLengthReq));
}

void GameClient::IncreaseReplaySpeed()
{
    if (replay_mode)
    {
        if (framesinfo.gf_length > 10)
            framesinfo.gf_length -= 10;
        else if (framesinfo.gf_length == 10)
            framesinfo.gf_length = 1;
    }
}

void GameClient::DecreaseReplaySpeed()
{
    if (replay_mode)
    {
        if (framesinfo.gf_length == 1)
            framesinfo.gf_length = 10;
        else if (framesinfo.gf_length < 1000)
            framesinfo.gf_length += 10;
    }
}


///////////////////////////////////////////////////////////////////////////////
/// NFC Done vom Server
/// @param message  Nachricht, welche ausgeführt wird
void GameClient::OnGameMessage(const GameMessage_Server_NWFDone& msg)
{
    // Emulate a push of the new gf length
    if(!framesinfo.gfLenghtNew)
        framesinfo.gfLenghtNew = msg.gf_length;
    else
    {
        RTTR_Assert(framesinfo.gfLenghtNew2 == 0);
        framesinfo.gfLenghtNew2 = msg.gf_length;
    }

    if(msg.first)
    {
        if(msg.nr % framesinfo.nwf_length == 0)
            framesinfo.gfNrServer = msg.nr; // If the next frame is a NWF, we allow only this one
        else
        {
            framesinfo.gfNrServer = msg.nr + framesinfo.nwf_length;
            framesinfo.gfNrServer -= framesinfo.gfNrServer % framesinfo.nwf_length; // Set the value of the next NWF, not some GFs after that
        }
        RTTR_Assert(framesinfo.gf_length == msg.gf_length);
    }else
    {
        RTTR_Assert(framesinfo.gfNrServer == msg.nr); // We expect the next message when the server is at a NWF
        framesinfo.gfNrServer = msg.nr + framesinfo.nwf_length;
        framesinfo.gfNrServer -= framesinfo.gfNrServer % framesinfo.nwf_length; // Set the value of the next NWF, not some GFs after that
    }

    //LOG.write("framesinfo.gf_nr(%d) == framesinfo.gfNrServer(%d)\n", framesinfo.gf_nr, framesinfo.gfNrServer);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  NFC Pause-Nachricht von Server
 *
 *  @param[in] message Nachricht, welche ausgeführt wird
 *
 *  @author FloSoft
 */
void GameClient::OnGameMessage(const GameMessage_Pause& msg)
{
    //framesinfo.pause =  msg.paused;
    if(msg.paused)
        framesinfo.pause_gf = msg.nr;
    else
    {
        framesinfo.isPaused =  false;
        framesinfo.pause_gf = 0;
    }

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
void GameClient::OnGameMessage(const GameMessage_GetAsyncLog&  /*msg*/)
{
    // AsyncLog an den Server senden

    std::vector<RandomEntry> async_log = RANDOM.GetAsyncLog();

    // stückeln...
    std::vector<RandomEntry> part;
    for (std::vector<RandomEntry>::iterator it = async_log.begin(); it != async_log.end(); ++it)
    {
        part.push_back(*it);

        if (part.size() == 10)
        {
            send_queue.push(new GameMessage_SendAsyncLog(part, false));
            part.clear();
        }
    }

    send_queue.push(new GameMessage_SendAsyncLog(part, true));
}

/// Findet heraus, ob ein Spieler laggt und setzt bei diesen Spieler den entsprechenden flag
bool GameClient::IsPlayerLagging()
{
    bool is_lagging = false;

    for(unsigned char i = 0; i < players.getCount(); ++i)
    {
        if(players[i].isUsed())
        {
            if(players[i].gc_queue.empty())
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
    if ((framesinfo.gf_nr - 1) % 750 > 0)
        return;

    for (unsigned int i = 0; i < players.getCount(); ++i)
    {
        players[i].StatisticStep();
    }

    // Check objective if there is one and there are at least two players
    if (ggs.game_objective == GlobalGameSettings::GO_NONE)
        return;

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
            gw->GetGameInterface()->GI_Winner(best);
        else
            gw->GetGameInterface()->GI_TeamWinner(bestteam);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// testet ob ein Netwerkframe abgelaufen ist und führt dann ggf die Befehle aus
void GameClient::ExecuteGameFrame(const bool skipping)
{
    unsigned int currentTime = VIDEODRIVER.GetTickCount();
    if(!framesinfo.isPaused && framesinfo.pause_gf != 0 && framesinfo.gf_nr == framesinfo.pause_gf)
    {
        framesinfo.pause_gf = 0;
        framesinfo.isPaused = true;
    }

    if(framesinfo.isPaused)
        return; // Pause

    if(framesinfo.forcePauseLen)
    {
        if(currentTime - framesinfo.forcePauseStart > framesinfo.forcePauseLen)
            framesinfo.forcePauseLen = 0;
        else
            return; // Pause
    }

    // Is it time for the next GF? If we are skipping, it is always time for the next GF
    if(skipping || skiptogf > framesinfo.gf_nr || (currentTime - framesinfo.lastTime) > framesinfo.gf_length)
    {
        if(replay_mode)
        {
            // In replay mode we have all commands in  the file -> Execute them
            ExecuteGameFrame_Replay();
        }else
        {
            // Is it time for a NWF, handle that first
            if(framesinfo.gf_nr % framesinfo.nwf_length == 0)
            {
                // If a player is lagging (we did not got his commands) "pause" the game by skipping the rest of this function
                // -> Don't execute GF, don't autosave etc.
                if(IsPlayerLagging())
                {
                    // If a player is a few GFs behind, he will never catch up and always lag
                    // Hence, pause up to 4 GFs randomly before trying again to execute this NWF
                    // Do not reset frameTime or lastTime as this will mess up interpolation for drawing
                    framesinfo.forcePauseStart = currentTime;
                    framesinfo.forcePauseLen = (rand() * 4 * framesinfo.gf_length) / RAND_MAX;
                    return;
                }

                // Same for the server
                if(framesinfo.gfNrServer < framesinfo.gf_nr)
                    return;
                RTTR_Assert(framesinfo.gfNrServer <= framesinfo.gf_nr + framesinfo.nwf_length);

                ExecuteNWF();

                RTTR_Assert(framesinfo.gfLenghtNew != 0);
                if(framesinfo.gfLenghtNew != framesinfo.gf_length)
                {
                    unsigned oldGfLen = framesinfo.gf_length;
                    int oldNwfLen = framesinfo.nwf_length;
                    framesinfo.ApplyNewGFLength();
                    framesinfo.gfLengthReq = framesinfo.gf_length;

                    // Adjust next confirmation for next NWF (if we have it already)
                    if(framesinfo.gfNrServer != framesinfo.gf_nr)
                    {
                        RTTR_Assert(framesinfo.gfNrServer == framesinfo.gf_nr + oldNwfLen);
                        framesinfo.gfNrServer = framesinfo.gfNrServer - oldNwfLen + framesinfo.nwf_length;
                        // Make it a NWF (mostly for validation and consistency)
                        framesinfo.gfNrServer -= framesinfo.gfNrServer % framesinfo.nwf_length;
                    }

                    LOG.lprintf("Client: %u/%u: Speed changed from %u to %u (NWF: %u to %u)\n", framesinfo.gfNrServer, framesinfo.gf_nr, oldGfLen, framesinfo.gf_length, oldNwfLen, framesinfo.nwf_length);
                }
                // "pop" the length
                framesinfo.gfLenghtNew = framesinfo.gfLenghtNew2;
                framesinfo.gfLenghtNew2 = 0;
            }

            NextGF();
        }

        RTTR_Assert(replay_mode || framesinfo.gf_nr <= framesinfo.gfNrServer + framesinfo.nwf_length);
        // Store this timestamp
        framesinfo.lastTime = currentTime;
        // Reset frameTime
        framesinfo.frameTime = 0;

        HandleAutosave();

        // GF-Ende im Replay aktualisieren
        if(!replay_mode)
            replayinfo.replay.UpdateLastGF(framesinfo.gf_nr);
    }else
    {
        // Next GF not yet reached, just update the time in the current one for drawing
        framesinfo.frameTime = currentTime - framesinfo.lastTime;
        RTTR_Assert(framesinfo.frameTime <= framesinfo.gf_length);
    }
}

void GameClient::HandleAutosave()
{
    // If inactive or during replay -> no autosave
    if(!SETTINGS.interface.autosave_interval  || replay_mode)
        return;

    // Alle .... GF
    if(framesinfo.gf_nr % SETTINGS.interface.autosave_interval == 0)
    {
        std::string tmp = GetFilePath(FILE_PATHS[85]);

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

        SaveToFile(tmp);
    }
}

/// Führt notwendige Dinge für nächsten GF aus
void GameClient::NextGF()
{
    ++framesinfo.gf_nr;
    // Statistiken aktualisieren
    StatisticStep();
    //  EventManager Bescheid sagen
    em->NextGF();
    // Notfallprogramm durchlaufen lassen
    for(unsigned char i = 0; i < players.getCount(); ++i)
    {
        if(players[i].isUsed())
        {
            // Auf Notfall testen (Wenige Bretter/Steine und keine Holzindustrie)
            players[i].TestForEmergencyProgramm();
            // Bündnisse auf Aktualität überprüfen
            players[i].TestPacts();
        }
    }
    
    if (human_ai)
    {
        human_ai->RunGF(framesinfo.gf_nr, (framesinfo.gf_nr % framesinfo.nwf_length == 0));

        const std::vector<gc::GameCommandPtr>& ai_gcs = human_ai->GetGameCommands();
        gameCommands_.insert(gameCommands_.end(), ai_gcs.begin(), ai_gcs.end());
        human_ai->FetchGameCommands();
    }
    
    if(gw->HasLua())
        gw->GetLua().EventGameFrame(framesinfo.gf_nr);
}


void GameClient::ExecuteAllGCs(const GameMessage_GameCommand& gcs)
{
    for(unsigned char i = 0; i < gcs.gcs.size(); ++i)
    {
        // NC ausführen
        gcs.gcs[i]->Execute(*gw, players[gcs.player], gcs.player);
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
        checksum = RANDOM.GetCurrentRandomValue();

    send_queue.push(new GameMessage_GameCommand(playerId_, AsyncChecksum(checksum), std::vector<gc::GameCommandPtr>()));
}

void GameClient::WritePlayerInfo(SavedFile& file)
{
    // Spieleranzahl
    file.SetPlayerCount(players.getCount());

    // Spielerdaten
    for(unsigned i = 0; i < players.getCount(); ++i)
    {
        SavedFile::Player& player = file.GetPlayer(i);
        player.ps = unsigned(players[i].ps);

        if(players[i].ps != PS_LOCKED)
        {
            player.name = players[i].name;
            player.nation = players[i].nation;
            player.color = players[i].color;
            player.team = players[i].team;
        }
    }
}

void GameClient::WriteReplayHeader(const unsigned random_init)
{
    // Dateiname erzeugen
    unser_time_t cTime = TIME.CurrentTime();
    std::string fileName = GetFilePath(FILE_PATHS[51]) + TIME.FormatTime("%Y-%m-%d_%H-%i-%s", &cTime) + ".rpl";

    // Headerinfos füllen

    // Timestamp der Aufzeichnung
    replayinfo.replay.save_time = cTime;
    /// NWF-Länge
    replayinfo.replay.nwf_length = framesinfo.nwf_length;
    // Random-Init
    replayinfo.replay.random_init = random_init;

    WritePlayerInfo(replayinfo.replay);

    // GGS-Daten
    replayinfo.replay.ggs = ggs;

    // Mapname
    replayinfo.replay.mapName = mapinfo.title;
    replayinfo.replay.mapFileName = bfs::path(mapinfo.filepath).filename().string();

    // Datei speichern
    if(!replayinfo.replay.WriteHeader(fileName, mapinfo))
        LOG.lprintf("GameClient::WriteReplayHeader: WARNING: File couldn't be opened. Don't use a replayinfo.replay.\n");
}

unsigned GameClient::StartReplay(const std::string& path, GameWorldViewer*& gwv)
{
    replayinfo.Clear();
    mapinfo.Clear();
    replayinfo.filename = path;

    if(!replayinfo.replay.LoadHeader(path, &mapinfo))
        return 2;

    // NWF-Länge
    framesinfo.nwf_length = replayinfo.replay.nwf_length;

    // Spielerdaten
    for(unsigned char i = 0; i < replayinfo.replay.GetPlayerCount(); ++i)
    {
        players.push_back(GameClientPlayer(i));

        const SavedFile::Player& player = replayinfo.replay.GetPlayer(i);
        players[i].ps = PlayerState(player.ps);

        if(players[i].ps != PS_LOCKED)
        {
            players[i].name = player.name;
            players[i].nation = player.nation;
            players[i].color = player.color;
            players[i].team = Team(player.team);
        }
    }

    bool playerFound = false;
    // Find a player to spectate from
    // First find a human player
    for(unsigned char i = 0; i < players.getCount(); ++i)
    {
        if(players[i].ps == PS_OCCUPIED)
        {
            playerId_ = i;
            playerFound = true;
            break;
        }
    }
    if(!playerFound)
    {
        // If no human found, take the first AI
        for(unsigned char i = 0; i < players.getCount(); ++i)
        {
            if(players[i].ps == PS_KI)
            {
                playerId_ = i;
                break;
            }
        }
    }

    // GGS-Daten
    ggs = replayinfo.replay.ggs;

    switch(mapinfo.type)
    {
        default:
            break;
        case MAPTYPE_OLDMAP:
        {
            // Richtigen Pfad zur Map erstellen
            mapinfo.filepath = GetFilePath(FILE_PATHS[48]) +  replayinfo.replay.mapFileName;
            if(!mapinfo.mapData.DecompressToFile(mapinfo.filepath))
            {
                if(ci)
                    ci->CI_Error(CE_WRONGMAP);
                Stop();
                return 2;
            }
            if(mapinfo.luaData.length)
            {
                mapinfo.luaFilepath = mapinfo.filepath.substr(0, mapinfo.filepath.length() - 3) + "lua";
                if(!mapinfo.luaData.DecompressToFile(mapinfo.luaFilepath))
                {
                    if(ci)
                        ci->CI_Error(CE_WRONGMAP);
                    Stop();
                    return 2;
                }
            }
        } break;
        case MAPTYPE_SAVEGAME:
            break;
    }

    replay_mode = true;
    replayinfo.async = 0;
    replayinfo.end = false;

    try
    {
        StartGame(replayinfo.replay.random_init);
    }
    catch (SerializedGameData::Error& error)
    {
        LOG.lprintf("Error when loading game: %s\n", error.what());
        if(ci)
            ci->CI_Error(CE_WRONGMAP);
        Stop();
        return 1;
    }

    replayinfo.replay.ReadGF(&replayinfo.next_gf);

    gwv = gw;
    state = CS_LOADING;

    return 0;
}

unsigned int GameClient::GetGlobalAnimation(const unsigned short max, const unsigned char factor_numerator, const unsigned char factor_denumerator, const unsigned int offset)
{
    // Unit for animations is 630ms (dividable by 2, 3, 5, 6, 7, 10, 15, ...)
    // But this also means: If framerate drops below approx. 15Hz, you won't see
    // every frame of an 8-part animation anymore.
    // An animation runs fully in (factor_numerator / factor_denumerator) multiples of 630ms
    const unsigned unit = 630/*ms*/ * factor_numerator / factor_denumerator;
    // Good approximation of current time in ms
    // (Accuracy of a possibly expensive VideoDriverWrapper::GetTicks() isn't needed here):
    const unsigned currenttime = framesinfo.lastTime + framesinfo.frameTime;
    return ((currenttime % unit) * max / unit + offset) % max;
}

unsigned GameClient::Interpolate(unsigned max_val, EventManager::EventPointer ev)
{
    RTTR_Assert( ev );
    return min<unsigned int>(((max_val * ((framesinfo.gf_nr - ev->gf) * framesinfo.gf_length + framesinfo.frameTime)) / (ev->gf_length * framesinfo.gf_length)), max_val - 1);
}

int GameClient::Interpolate(int x1, int x2, EventManager::EventPointer ev)
{
    RTTR_Assert( ev );
    return (x1 + ( (x2 - x1) * ((int(framesinfo.gf_nr) - int(ev->gf)) * int(framesinfo.gf_length) + int(framesinfo.frameTime))) / int(ev->gf_length * framesinfo.gf_length));
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
void GameClient::SkipGF(unsigned int gf, GameWorldView& gwv)
{
    if(gf <= framesinfo.gf_nr)
        return;

    unsigned start_ticks = VIDEODRIVER.GetTickCount();

    // Spiel entpausieren
    if(replay_mode)
        SetReplayPause(false);
    if(!replay_mode)
    {
        //unpause before skipping
        if(GAMESERVER.IsPaused())
        {
            GAMESERVER.TogglePause();
            //return;
        }
        GAMESERVER.skiptogf=gf;
        skiptogf=gf;
        LOG.lprintf("jumping from gf %i to gf %i \n", framesinfo.gf_nr, gf);
        return;
    }
    

    // GFs überspringen
    for(unsigned int i = framesinfo.gf_nr; i < gf;++i)
    {
        if(i % 1000 == 0)
        {
            RoadBuildState road;
            road.mode = RM_DISABLED;

            // spiel aktualisieren
            gwv.Draw(road);

            // text oben noch hinschreiben
            char nwf_string[256];
            snprintf(nwf_string, 255, _("current GF: %u - still fast forwarding: %d GFs left (%d %%)"), GetGFNumber(), gf - i, (i * 100 / gf) );
            LargeFont->Draw(VIDEODRIVER.GetScreenWidth() / 2, VIDEODRIVER.GetScreenHeight() / 2, nwf_string, glArchivItem_Font::DF_CENTER, 0xFFFFFF00);

            VIDEODRIVER.SwapBuffers();
        }
        ExecuteGameFrame(true);
        //LOG.lprintf("jumping: now at gf %i\n", framesinfo.nr);        
    }
    
    // Spiel pausieren & text ausgabe wie lang das jetzt gedauert hat 
    unsigned ticks = VIDEODRIVER.GetTickCount() - start_ticks;
    char text[256];
    snprintf(text, sizeof(text), _("Jump finished (%.3f seconds)."), (double) ticks / 1000.0);
    ci->CI_Chat(playerId_, CD_SYSTEM, text); 
    SetReplayPause(true);

}

void GameClient::SystemChat(const std::string& text, unsigned char player)
{
    if(player == 0xFF)
        player = playerId_;
    ci->CI_Chat(player, CD_SYSTEM, text);
}

unsigned GameClient::SaveToFile(const std::string& filename)
{
    GameMessage_System_Chat saveAnnouncement = GameMessage_System_Chat(playerId_, "Saving game...");
    send_queue.sendMessage(socket, saveAnnouncement);

    // Mond malen
    LOADER.GetImageN("resource", 33)->Draw(VIDEODRIVER.GetMouseX(), VIDEODRIVER.GetMouseY() - 40, 0, 0, 0, 0, 0, 0);
    VIDEODRIVER.SwapBuffers();

    Savegame save;

    // Timestamp der Aufzeichnung
    save.save_time = TIME.CurrentTime();
    // Mapname
    save.mapName = this->mapinfo.title;

    WritePlayerInfo(save);

    // GGS-Daten
    save.ggs = ggs;

    save.start_gf = framesinfo.gf_nr;

    // Enable/Disable debugging of savegames
    save.sgd.debugMode = SETTINGS.global.debugMode;

    // Spiel serialisieren
    save.sgd.MakeSnapshot(*gw, *em);

    // Und alles speichern
    if(!save.Save(filename))
        return 1;
    else
        return 0;
}

void GameClient::ResetVisualSettings()
{
    GameClientPlayer& player = GetLocalPlayer();
    //visual_settings.transport_order[0] = player.transport[GD_COINS];
    //visual_settings.transport_order[1] = player.transport[GD_SWORD];
    //visual_settings.transport_order[2] = player.transport[GD_BEER];
    //visual_settings.transport_order[3] = player.transport[GD_IRON];
    //visual_settings.transport_order[4] = player.transport[GD_GOLD];
    //visual_settings.transport_order[5] = player.transport[GD_IRONORE];
    //visual_settings.transport_order[6] = player.transport[GD_COAL];
    //visual_settings.transport_order[7] = player.transport[GD_BOARDS];
    //visual_settings.transport_order[8] = player.transport[GD_STONES];
    //visual_settings.transport_order[9] = player.transport[GD_WOOD];
    //visual_settings.transport_order[10] = player.transport[GD_WATER];
    //visual_settings.transport_order[11] = player.transport[GD_FISH];
    //visual_settings.transport_order[12] = player.transport[GD_HAMMER];
    //visual_settings.transport_order[13] = player.transport[GD_BOAT];

    visual_settings.transport_order[player.transport[GD_COINS]] = 0;
    visual_settings.transport_order[player.transport[GD_SWORD]] = 1;
    visual_settings.transport_order[player.transport[GD_BEER]] = 2;
    visual_settings.transport_order[player.transport[GD_IRON]] = 3;
    visual_settings.transport_order[player.transport[GD_GOLD]] = 4;
    visual_settings.transport_order[player.transport[GD_IRONORE]] = 5;
    visual_settings.transport_order[player.transport[GD_COAL]] = 6;
    visual_settings.transport_order[player.transport[GD_BOARDS]] = 7;
    visual_settings.transport_order[player.transport[GD_STONES]] = 8;
    visual_settings.transport_order[player.transport[GD_WOOD]] = 9;
    visual_settings.transport_order[player.transport[GD_WATER]] = 10;
    visual_settings.transport_order[player.transport[GD_FISH]] = 11;
    visual_settings.transport_order[player.transport[GD_HAMMER]] = 12;
    visual_settings.transport_order[player.transport[GD_BOAT]] = 13;



    visual_settings.distribution[0] = player.distribution[GD_FISH].percent_buildings[BLD_GRANITEMINE]; //-V807
    visual_settings.distribution[1] = player.distribution[GD_FISH].percent_buildings[BLD_COALMINE];
    visual_settings.distribution[2] = player.distribution[GD_FISH].percent_buildings[BLD_IRONMINE];
    visual_settings.distribution[3] = player.distribution[GD_FISH].percent_buildings[BLD_GOLDMINE];

    visual_settings.distribution[4] = player.distribution[GD_GRAIN].percent_buildings[BLD_MILL]; //-V807
    visual_settings.distribution[5] = player.distribution[GD_GRAIN].percent_buildings[BLD_PIGFARM];
    visual_settings.distribution[6] = player.distribution[GD_GRAIN].percent_buildings[BLD_DONKEYBREEDER];
    visual_settings.distribution[7] = player.distribution[GD_GRAIN].percent_buildings[BLD_BREWERY];
    visual_settings.distribution[8] = player.distribution[GD_GRAIN].percent_buildings[BLD_CHARBURNER];

    visual_settings.distribution[9] = player.distribution[GD_IRON].percent_buildings[BLD_ARMORY];
    visual_settings.distribution[10] = player.distribution[GD_IRON].percent_buildings[BLD_METALWORKS];

    visual_settings.distribution[11] = player.distribution[GD_COAL].percent_buildings[BLD_ARMORY]; //-V807
    visual_settings.distribution[12] = player.distribution[GD_COAL].percent_buildings[BLD_IRONSMELTER];
    visual_settings.distribution[13] = player.distribution[GD_COAL].percent_buildings[BLD_MINT];

    visual_settings.distribution[14] = player.distribution[GD_WOOD].percent_buildings[BLD_SAWMILL];
    visual_settings.distribution[15] = player.distribution[GD_WOOD].percent_buildings[BLD_CHARBURNER];

    visual_settings.distribution[16] = player.distribution[GD_BOARDS].percent_buildings[BLD_HEADQUARTERS]; //-V807
    visual_settings.distribution[17] = player.distribution[GD_BOARDS].percent_buildings[BLD_METALWORKS];
    visual_settings.distribution[18] = player.distribution[GD_BOARDS].percent_buildings[BLD_SHIPYARD];

    visual_settings.distribution[19] = player.distribution[GD_WATER].percent_buildings[BLD_BAKERY]; //-V807
    visual_settings.distribution[20] = player.distribution[GD_WATER].percent_buildings[BLD_BREWERY];
    visual_settings.distribution[21] = player.distribution[GD_WATER].percent_buildings[BLD_PIGFARM];
    visual_settings.distribution[22] = player.distribution[GD_WATER].percent_buildings[BLD_DONKEYBREEDER];


    visual_settings.military_settings = player.militarySettings_;
    visual_settings.tools_settings = player.toolsSettings_;

    visual_settings.order_type = player.orderType_;
    visual_settings.build_order = player.build_order;
}


void GameClient::SetReplayPause(bool pause)
{
    if(replay_mode)
    {
        framesinfo.isPaused = pause;
        framesinfo.frameTime = 0;
    }
}


bool GameClient::AddGC(gc::GameCommand* gc)
{
    // Nicht in der Pause oder wenn er besiegt wurde
    if(framesinfo.isPaused || GetLocalPlayer().isDefeated())
        return false;

    gameCommands_.push_back(gc);
    return true;
}

bool GameClient::IsSinglePlayer() const
{
    bool foundPlayer = false;
    for(GameClientPlayerList::const_iterator it = players.begin(); it != players.end(); ++it)
    {
        if(it->ps == PS_OCCUPIED)
        {
            if(foundPlayer)
                return false;
            else
                foundPlayer = true;
        }
    }
    return true;
}

/// Erzeugt einen KI-Player, der mit den Daten vom GameClient gefüttert werden muss (zusätzlich noch mit den GameServer)
AIBase* GameClient::CreateAIPlayer(const unsigned playerid)
{
    switch (players[playerid].aiInfo.type)
    {
    case AI::DUMMY:
        {
            return new AIPlayer(playerid, *gw, players[playerid], players, ggs, AI::EASY);
        } break;
    case AI::DEFAULT:
        {
            return new AIPlayerJH(playerid, *gw, players[playerid], players, ggs, players[playerid].aiInfo.level);
        } break;
        default:
        {
            return new AIPlayer(playerid, *gw, players[playerid], players, ggs, AI::EASY);
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
        DeletePostMessage(postMessages.back());
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

bool GameClient::SendAIEvent(AIEvent::Base* ev, unsigned receiver)
{
    if (human_ai && playerId_ == receiver)
    {
        human_ai->SendAIEvent(ev);
        return true;
    }
    
    if (IsHost())
        return GAMESERVER.SendAIEvent(ev, receiver);
    else
    {
        delete ev;
        return true;
    }
}

/// Schreibt ggf. Pathfinding-Results in das Replay, falls erforderlich
void GameClient::AddPathfindingResult(const unsigned char dir, const unsigned* const length, const MapPoint* const next_harbor)
{
    // Sind wir im normalem Spiel?
    if(!replay_mode || !replayinfo.replay.pathfinding_results)
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
    if(replay_mode || !replayinfo.replay.pathfinding_results)
    {
        // Unterstützt das Replay das auch (noch)?
        if(replayinfo.replay.pathfinding_results && !replayinfo.end)
            return true;
    }

    return false;
}

/// Gibt Pathfinding-Results zurück aus einem Replay
bool GameClient::ReadPathfindingResult(unsigned char* dir, unsigned* length, MapPoint* next_harbor)
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

void GameClient::LoadGGS()
{
    std::cout << "Loading settings...";
    ggs.LoadSettings();
    std::cout << "Done." << std::endl;
}

void GameClient::ToggleHumanAIPlayer()
{
    RTTR_Assert(!GAMECLIENT.IsReplayModeOn());
    if (human_ai)
    {
        delete human_ai;
        human_ai = NULL;
    } else
    {
        human_ai = new AIPlayerJH(playerId_, *gw, players[playerId_], players, ggs, AI::EASY);
    }
}

void GameClient::RequestSwapToPlayer(const unsigned char newId)
{
    if(state != CS_GAME)
        return;
    GameClientPlayer& player = GAMECLIENT.GetPlayer(newId);
    if(player.ps == PS_KI && player.aiInfo.type == AI::DUMMY)
        send_queue.push(new GameMessage_Player_Swap(playerId_, newId));
}
