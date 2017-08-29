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
#include "GameClient.h"
#include "RTTR_Version.h"

#include "ClientInterface.h"
#include "EventManager.h"
#include "GameEvent.h"
#include "GameInterface.h"
#include "GameLobby.h"
#include "GameManager.h"
#include "GameMessages.h"
#include "GameObject.h"
#include "GameServer.h"
#include "GlobalGameSettings.h"
#include "GlobalVars.h"
#include "JoinPlayerInfo.h"
#include "Loader.h"
#include "Random.h"
#include "Savegame.h"
#include "SerializedGameData.h"
#include "Settings.h"
#include "addons/const_addons.h"
#include "ai/AIPlayer.h"
#include "ai/AIPlayerJH.h"
#include "drivers/VideoDriverWrapper.h"
#include "files.h"
#include "helpers/Deleter.h"
#include "lua/LuaInterfaceGame.h"
#include "ogl/glArchivItem_Font.h"
#include "ogl/glArchivItem_Map.h"
#include "postSystem/PostManager.h"
#include "world/GameWorld.h"
#include "world/GameWorldView.h"
#include "gameTypes/RoadBuildState.h"
#include "gameData/GameConsts.h"
#include "libsiedler2/src/ArchivItem_Map_Header.h"
#include "libsiedler2/src/prototypen.h"
#include "libutil/src/SocketSet.h"
#include "libutil/src/fileFuncs.h"
#include <boost/filesystem.hpp>
#include <boost/interprocess/smart_ptr/unique_ptr.hpp>
#include <boost/smart_ptr/scoped_array.hpp>
#include <cerrno>
#include <fstream>
#include <iostream>

void GameClient::ClientConfig::Clear()
{
    server.clear();
    gameName.clear();
    password.clear();
    port = 0;
    isHost = false;
}

void GameClient::RandCheckInfo::Clear()
{
    rand = 0;
}

void GameClient::ReplayInfo::Clear()
{
    replay = Replay();
    async = 0;
    end = false;
    next_gf = 0;
    filename.clear();
    all_visible = false;
}

GameClient::GameClient()
    : skiptogf(0), playerId_(0), recv_queue(&GameMessage::create_game), send_queue(&GameMessage::create_game), state(CS_STOPPED), ci(NULL),
      replay_mode(false), game_log(NULL)
{
    clientconfig.Clear();
    framesinfo.Clear();
    randcheckinfo.Clear();
}

GameClient::~GameClient()
{
    Stop();
}

/**
 *  Verbindet den Client mit einem Server
 *
 *  @param server    Hostname des Zielrechners
 *  @param password  Passwort des Spieles
 *  @param servertyp Servertyp des Spieles (Direct/LAN/usw)
 *  @param host      gibt an ob wir selbst der Host sind
 *
 *  @return true, wenn Client erfolgreich verbunden und gestartet
 */
bool GameClient::Connect(const std::string& server, const std::string& password, ServerType servertyp, unsigned short port, bool host,
                         bool use_ipv6)
{
    Stop();

    // Name und Password kopieren
    clientconfig.server = server;
    clientconfig.password = password;

    clientconfig.servertyp = servertyp;
    clientconfig.port = port;
    clientconfig.isHost = host;

    // Verbinden
    if(!socket.Connect(server, port, use_ipv6, (Socket::PROXY_TYPE)SETTINGS.proxy.typ, SETTINGS.proxy.proxy, SETTINGS.proxy.port)) //-V807
    {
        LOG.write("GameClient::Connect: ERROR: Connect failed!\n");
        return false;
    }

    state = CS_CONNECT;

    if(ci)
        ci->CI_NextConnectState(CS_WAITFORANSWER);

    // Es wird kein Replay abgespielt, sondern dies ist ein richtiges Spiel
    replay_mode = false;

    return true;
}

/**
 *  Hauptschleife des Clients
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
            LOG.write("Receiving Message from server failed\n");
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
            LOG.write("Error on socket to server\n");
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

/**
 *  Stoppt das Spiel
 */
void GameClient::Stop()
{
    if(state == CS_STOPPED)
        return;

    if(state == CS_LOADING || state == CS_GAME)
        ExitGame();
    else if(state == CS_CONNECT || state == CS_CONFIG)
        gameLobby.reset();

    if(IsHost())
        GAMESERVER.Stop();

    framesinfo.Clear();
    clientconfig.Clear();
    mapinfo.Clear();

    replayinfo.replay.StopRecording();

    socket.Close();

    // clear jump target
    skiptogf = 0;

    // Consistency check: No world, no lobby remaining
    RTTR_Assert(!gw);
    RTTR_Assert(!gameLobby);

    state = CS_STOPPED;
    LOG.write("client state changed to stop\n");
}

GameLobby& GameClient::GetGameLobby()
{
    RTTR_Assert(state == CS_CONFIG);
    RTTR_Assert(gameLobby);
    return *gameLobby;
}

/**
 *  Startet ein Spiel oder Replay.
 *
 *  @param[in] random_init Initialwert des Zufallsgenerators.
 */
void GameClient::StartGame(const unsigned random_init)
{
    RTTR_Assert(state == CS_CONFIG || (state == CS_STOPPED && replay_mode));

    // Mond malen
    Point<int> moonPos = VIDEODRIVER.GetMousePos();
    moonPos.y -= 40;
    LOADER.GetImageN("resource", 33)->DrawFull(moonPos);
    VIDEODRIVER.SwapBuffers();

    // Daten zurücksetzen
    randcheckinfo.Clear();

    // Start in pause mode
    framesinfo.isPaused = true;

    // Je nach Geschwindigkeit GF-Länge einstellen
    framesinfo.gf_length = SPEED_GF_LENGTHS[gameLobby->GetSettings().speed];
    framesinfo.gfLengthReq = framesinfo.gf_length;

    // Random-Generator initialisieren
    RANDOM.Init(random_init);

    // If we have a savegame, start at its first GF, else at 0
    unsigned startGF = (mapinfo.type == MAPTYPE_SAVEGAME) ? mapinfo.savegame->start_gf : 0;
    // Create the world, starting with the event manager
    em.reset(new EventManager(startGF));
    // Store settings (only reference stored in World)
    ggs = gameLobby->GetSettings();
    gw.reset(new GameWorld(std::vector<PlayerInfo>(gameLobby->GetPlayers().begin(), gameLobby->GetPlayers().end()), ggs, *em));
    // Init data
    GameObject::SetPointers(gw.get());
    gw->GetPostMgr().AddPostBox(playerId_);
    // Release lobby
    gameLobby.reset();

    state = CS_LOADING;

    if(ci)
        ci->CI_GameStarted(*gw);

    // Get standard settings before they get overwritten
    GetPlayer(playerId_).FillVisualSettings(default_settings);

    if(mapinfo.savegame)
    {
        mapinfo.savegame->sgd.ReadSnapshot(*gw);
    } else
    {
        RTTR_Assert(mapinfo.type != MAPTYPE_SAVEGAME);
        /// Startbündnisse setzen
        for(unsigned i = 0; i < gw->GetPlayerCount(); ++i)
            gw->GetPlayer(i).MakeStartPacts();

        gw->LoadMap(mapinfo.filepath, mapinfo.luaFilepath);

        /// Evtl. Goldvorkommen ändern
        unsigned char target = 0xFF; // löschen
        switch(ggs.getSelection(AddonId::CHANGE_GOLD_DEPOSITS))
        {
            case 0:
                target = 3;
                break; // in Gold   konvertieren bzw. nichts tun
            case 1:
                target = 0xFF;
                break; // löschen
            case 2:
                target = 2;
                break; // in Eisen  konvertieren
            case 3:
                target = 1;
                break; // in Kohle  konvertieren
            case 4:
                target = 0;
                break; // in Granit konvertieren
        }
        if(target != 3)
            gw->ConvertMineResourceTypes(3, target);
    }
    gw->InitAfterLoad();

    // Update visual settings
    ResetVisualSettings();

    // Zeit setzen
    framesinfo.lastTime = VIDEODRIVER.GetTickCount();

    if(!replay_mode)
    {
        WriteReplayHeader(random_init);
        std::stringstream fileName;
        fileName << FILE_PATHS[47] << TIME.FormatTime("game_%Y-%m-%d_%H-%i-%s") << "-" << (rand() % 100) << ".log";

        game_log = fopen(fileName.str().c_str(), "a");
    }

    // Daten nach dem Schreiben des Replays ggf wieder löschen
    mapinfo.mapData.Clear();
}

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

void GameClient::ExitGame()
{
    RTTR_Assert(state == CS_GAME || state == CS_LOADING);
    GameObject::SetPointers(NULL);
    // Spielwelt zerstören
    human_ai.reset();
    gw.reset();
    em.reset();
    // Clear remaining commands
    gameCommands_.clear();
}

unsigned GameClient::GetGFNumber() const
{
    return em->GetCurrentGF();
}

/**
 *  Ping-Nachricht.
 */
void GameClient::OnGameMessage(const GameMessage_Ping& /*msg*/)
{
    send_queue.push(new GameMessage_Pong(0xFF));
}

/**
 *  Player-ID-Nachricht.
 */
void GameClient::OnGameMessage(const GameMessage_Player_Id& msg)
{
    // haben wir eine ungültige ID erhalten? (aka Server-Voll)
    if(msg.playerId == 0xFFFFFFFF)
    {
        if(ci)
            ci->CI_Error(CE_SERVERFULL);

        Stop();
        return;
    }

    this->playerId_ = msg.playerId;

    // Server-Typ senden
    send_queue.push(new GameMessage_Server_Type(clientconfig.servertyp, RTTR_Version::GetVersion()));
}

/**
 *  Player-List-Nachricht.
 */
void GameClient::OnGameMessage(const GameMessage_Player_List& msg)
{
    RTTR_Assert(state == CS_CONNECT);
    RTTR_Assert(gameLobby);
    RTTR_Assert(gameLobby->GetPlayerCount() == msg.playerInfos.size());

    for(unsigned i = 0; i < gameLobby->GetPlayerCount(); ++i)
    {
        JoinPlayerInfo& playerInfo = gameLobby->GetPlayer(i);
        playerInfo = msg.playerInfos[i];
        if(playerInfo.ps == PS_AI)
            playerInfo.isReady = true;
    }

    state = CS_CONFIG;
    if(ci)
        ci->CI_NextConnectState(CS_FINISHED);
}

///////////////////////////////////////////////////////////////////////////////
/// player joined
/// @param message  Nachricht, welche ausgeführt wird
inline void GameClient::OnGameMessage(const GameMessage_Player_New& msg)
{
    LOG.writeToFile("<<< NMS_PLAYER_NEW(%d)\n") % msg.player;
    RTTR_Assert(state == CS_CONFIG);
    if(msg.player >= gameLobby->GetPlayerCount())
        return;

    JoinPlayerInfo& playerInfo = gameLobby->GetPlayer(msg.player);

    playerInfo.name = msg.name;
    playerInfo.ps = PS_OCCUPIED;
    playerInfo.ping = 0;
    playerInfo.InitRating();

    if(ci)
        ci->CI_NewPlayer(msg.player);
}

void GameClient::OnGameMessage(const GameMessage_Player_Ping& msg)
{
    if(state == CS_CONFIG)
    {
        if(msg.player >= gameLobby->GetPlayerCount())
            return;
        gameLobby->GetPlayer(msg.player).ping = msg.ping;
    } else if(state == CS_LOADING || state == CS_GAME)
    {
        if(msg.player >= GetPlayerCount())
            return;
        GetPlayer(msg.player).ping = msg.ping;
    } else
    {
        RTTR_Assert(false);
        return;
    }

    if(ci)
        ci->CI_PingChanged(msg.player, msg.ping);
}

/**
 *  Player-Toggle-State-Nachricht.
 */
void GameClient::OnGameMessage(const GameMessage_Player_Set_State& msg)
{
    RTTR_Assert(state == CS_CONFIG);
    if(msg.player >= gameLobby->GetPlayerCount())
        return;

    JoinPlayerInfo& playerInfo = gameLobby->GetPlayer(msg.player);
    playerInfo.ps = msg.ps;
    playerInfo.aiInfo = msg.aiInfo;
    playerInfo.InitRating();
    if(playerInfo.ps == PS_AI)
        playerInfo.SetAIName(msg.player);

    playerInfo.isReady = (playerInfo.ps == PS_AI);
    if(ci)
    {
        ci->CI_PSChanged(msg.player, playerInfo.ps);
        ci->CI_ReadyChanged(msg.player, playerInfo.isReady);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// nation button gedrückt
/// @param message  Nachricht, welche ausgeführt wird
inline void GameClient::OnGameMessage(const GameMessage_Player_Set_Nation& msg)
{
    RTTR_Assert(state == CS_CONFIG);
    if(msg.player >= gameLobby->GetPlayerCount())
        return;

    gameLobby->GetPlayer(msg.player).nation = msg.nation;

    if(ci)
        ci->CI_NationChanged(msg.player, msg.nation);
}

///////////////////////////////////////////////////////////////////////////////
/// team button gedrückt
/// @param message  Nachricht, welche ausgeführt wird
inline void GameClient::OnGameMessage(const GameMessage_Player_Set_Team& msg)
{
    RTTR_Assert(state == CS_CONFIG);
    if(msg.player >= gameLobby->GetPlayerCount())
        return;

    gameLobby->GetPlayer(msg.player).team = msg.team;

    if(ci)
        ci->CI_TeamChanged(msg.player, msg.team);
}

///////////////////////////////////////////////////////////////////////////////
/// color button gedrückt
/// @param message  Nachricht, welche ausgeführt wird
inline void GameClient::OnGameMessage(const GameMessage_Player_Set_Color& msg)
{
    RTTR_Assert(state == CS_CONFIG);
    if(msg.player >= gameLobby->GetPlayerCount())
        return;

    gameLobby->GetPlayer(msg.player).color = msg.color;

    if(ci)
        ci->CI_ColorChanged(msg.player, msg.color);
}

/**
 *  Ready-state eines Spielers hat sich geändert.
 *
 *  @param[in] message Nachricht, welche ausgeführt wird
 */
inline void GameClient::OnGameMessage(const GameMessage_Player_Ready& msg)
{
    LOG.writeToFile("<<< NMS_PLAYER_READY(%d, %s)\n") % msg.player % (msg.ready ? "true" : "false");

    RTTR_Assert(state == CS_CONFIG);
    if(msg.player >= gameLobby->GetPlayerCount())
        return;

    gameLobby->GetPlayer(msg.player).isReady = msg.ready;

    if(ci)
        ci->CI_ReadyChanged(msg.player, msg.ready);
}

///////////////////////////////////////////////////////////////////////////////
/// player gekickt
/// @param message  Nachricht, welche ausgeführt wird
inline void GameClient::OnGameMessage(const GameMessage_Player_Kicked& msg)
{
    LOG.writeToFile("<<< NMS_PLAYER_KICKED(%d, %d, %d)\n") % msg.player % msg.cause % msg.param;

    RTTR_Assert(state == CS_CONFIG || state == CS_LOADING || state == CS_GAME);

    if(state == CS_CONFIG)
    {
        if(msg.player >= gameLobby->GetPlayerCount())
            return;
        gameLobby->GetPlayer(msg.player).ps = PS_FREE;
    } else
    {
        // Im Spiel anzeigen, dass der Spieler das Spiel verlassen hat
        gw->GetPlayer(msg.player).ps = PS_AI;
    }

    if(ci)
        ci->CI_PlayerLeft(msg.player);
}

inline void GameClient::OnGameMessage(const GameMessage_Player_Swap& msg)
{
    LOG.writeToFile("<<< NMS_PLAYER_SWAP(%u, %u)\n") % msg.player % msg.player2;

    RTTR_Assert(state == CS_CONFIG || state == CS_LOADING || state == CS_GAME);

    if(state == CS_CONFIG)
    {
        if(msg.player >= gameLobby->GetPlayerCount() || msg.player2 >= gameLobby->GetPlayerCount())
            return;

        // During preparation just swap the players
        using std::swap;
        swap(gameLobby->GetPlayer(msg.player), gameLobby->GetPlayer(msg.player2));
        // Some things cannot be changed in savegames
        if(mapinfo.type == MAPTYPE_SAVEGAME)
            gameLobby->GetPlayer(msg.player).FixSwappedSaveSlot(gameLobby->GetPlayer(msg.player2));

        // Evtl. sind wir betroffen?
        if(playerId_ == msg.player)
            playerId_ = msg.player2;
        else if(playerId_ == msg.player2)
            playerId_ = msg.player;

        if(ci)
            ci->CI_PlayersSwapped(msg.player, msg.player2);
    } else
    {
        if(msg.player >= gw->GetPlayerCount() || msg.player2 >= gw->GetPlayerCount())
            return;
        ChangePlayerIngame(msg.player, msg.player2);
    }
}

/**
 *  Server-Typ-Nachricht.
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
        }
        break;

        case 2:
        {
            if(ci)
                ci->CI_Error(CE_WRONGVERSION);
            Stop();
            return;
        }
        break;
    }

    send_queue.push(new GameMessage_Server_Password(clientconfig.password));

    if(ci)
        ci->CI_NextConnectState(CS_QUERYPW);
}

/**
 *  Server-Passwort-Nachricht.
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

/**
 *  Server-Name-Nachricht.
 */
void GameClient::OnGameMessage(const GameMessage_Server_Name& msg)
{
    clientconfig.gameName = msg.name;

    if(ci)
        ci->CI_NextConnectState(CS_QUERYPLAYERLIST);
}

/**
 *  Server-Start-Nachricht
 */
inline void GameClient::OnGameMessage(const GameMessage_Server_Start& msg)
{
    // NWF-Länge bekommen wir vom Server
    framesinfo.nwf_length = msg.nwf_length;

    /// Beim Host muss das Spiel nicht nochmal gestartet werden, das hat der Server schon erledigt
    if(IsHost())
        return;

    try
    {
        StartGame(msg.random_init);
    } catch(SerializedGameData::Error& error)
    {
        LOG.write("Error when loading game: %s\n") % error.what();
        GAMEMANAGER.ShowMenu();
        ExitGame();
    }
}

/**
 *  Server-Chat-Nachricht.
 */
void GameClient::OnGameMessage(const GameMessage_Server_Chat& msg)
{
    if(state == CS_GAME)
    {
        // Ingame message: Do some checking and logging
        if(msg.player >= gw->GetPlayerCount())
            return;

        /// Mit im Replay aufzeichnen
        replayinfo.replay.AddChatCommand(GetGFNumber(), msg.player, msg.destination, msg.text);

        GamePlayer& player = gw->GetPlayer(msg.player);

        // Besiegte dürfen nicht mehr heimlich mit Verbüdeten oder Feinden reden
        if(player.IsDefeated() && msg.destination != CD_ALL)
            return;
        // Entscheiden, ob ich ein Gegner oder Vebündeter bin vom Absender
        bool ally = player.IsAlly(playerId_);

        // Chatziel unerscheiden und ggf. nicht senden
        if(!ally && msg.destination == CD_ALLIES)
            return;
        if(ally && msg.destination == CD_ENEMIES && msg.player != playerId_)
            return;
    } else if(state == CS_CONFIG)
    {
        // GameLobby message: Just check for valid player
        if(msg.player >= gameLobby->GetPlayerCount())
            return;
    } else
        return;

    if(ci)
        ci->CI_Chat(msg.player, msg.destination, msg.text);
}

void GameClient::OnGameMessage(const GameMessage_System_Chat& msg)
{
    SystemChat(msg.text, msg.player);
}

/**
 *  Server-Async-Nachricht.
 */
void GameClient::OnGameMessage(const GameMessage_Server_Async& msg)
{
    if(state != CS_GAME)
        return;

    // Liste mit Namen und Checksummen erzeugen
    std::stringstream checksum_list;
    for(unsigned i = 0; i < msg.checksums.size(); ++i)
    {
        checksum_list << gw->GetPlayer(i).name << ": " << msg.checksums[i];
        if(i + 1 < msg.checksums.size())
            checksum_list << ", ";
    }

    // Fehler ausgeben (Konsole)!
    LOG.write(_("The Game is not in sync. Checksums of some players don't match."));
    LOG.write(checksum_list.str());
    LOG.write("\n");

    // Messenger im Game
    if(ci)
        ci->CI_Async(checksum_list.str());

    std::string timeStr = TIME.FormatTime("async_%Y-%m-%d_%H-%i-%s");
    std::string filePathSave = GetFilePath(FILE_PATHS[85]) + timeStr + ".sav";
    std::string filePathLog = GetFilePath(FILE_PATHS[47]) + timeStr + "Player.log";
    RANDOM.SaveLog(filePathLog);
    SaveToFile(filePathSave);
    LOG.write("Async log saved at \"%s\", game saved at \"%s\"\n") % filePathLog % filePathSave;
}

/**
 *  Server-Countdown-Nachricht.
 */
void GameClient::OnGameMessage(const GameMessage_Server_Countdown& msg)
{
    if(ci)
        ci->CI_Countdown(msg.countdown);
}

/**
 *  Server-Cancel-Countdown-Nachricht.
 */
void GameClient::OnGameMessage(const GameMessage_Server_CancelCountdown& /*msg*/)
{
    if(ci)
        ci->CI_CancelCountdown();
}

/**
 *  verarbeitet die MapInfo-Nachricht, in der die gepackte Größe,
 *  die normale Größe und Teilanzahl der Karte übertragen wird.
 *
 *  @param message Nachricht, welche ausgeführt wird
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
    if(msg.luaLen > 0)
        mapinfo.luaFilepath = mapinfo.filepath.substr(0, mapinfo.filepath.length() - 3) + "lua";
    else
        mapinfo.luaFilepath.clear();
}

///////////////////////////////////////////////////////////////////////////////
/// Kartendaten
/// @param message  Nachricht, welche ausgeführt wird
inline void GameClient::OnGameMessage(const GameMessage_Map_Data& msg)
{
    RTTR_Assert(state == CS_CONNECT);
    LOG.writeToFile("<<< NMS_MAP_DATA(%u)\n") % msg.data.size();
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
                libsiedler2::Archiv map;

                // Karteninformationen laden
                if(libsiedler2::loader::LoadMAP(mapinfo.filepath, map, true) != 0)
                {
                    LOG.write("GameClient::OnMapData: ERROR: Map \"%s\", couldn't load header!\n") % mapinfo.filepath;
                    if(ci)
                        ci->CI_Error(CE_WRONGMAP);
                    Stop();
                    return;
                }

                const libsiedler2::ArchivItem_Map_Header* header = &(dynamic_cast<const glArchivItem_Map*>(map.get(0))->getHeader());
                RTTR_Assert(header);

                RTTR_Assert(!gameLobby);
                gameLobby.reset(new GameLobby(header->getPlayer()));
                mapinfo.title = cvStringToUTF8(header->getName());
            }
            break;
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

                RTTR_Assert(!gameLobby);
                gameLobby.reset(new GameLobby(mapinfo.savegame->GetPlayerCount()));
                mapinfo.title = mapinfo.savegame->mapName;
            }
            break;
        }

        if(playerId_ >= gameLobby->GetPlayerCount())
        {
            if(ci)
                ci->CI_Error(CE_WRONGMAP);
            Stop();
            return;
        }
        send_queue.push(new GameMessage_Map_Checksum(mapinfo.mapChecksum, mapinfo.luaChecksum));

        LOG.writeToFile(">>>NMS_MAP_CHECKSUM(%u)\n") % mapinfo.mapChecksum;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// map-checksum
/// @param message  Nachricht, welche ausgeführt wird
inline void GameClient::OnGameMessage(const GameMessage_Map_ChecksumOK& msg)
{
    LOG.writeToFile("<<< NMS_MAP_CHECKSUM(%d)\n") % (msg.correct ? 1 : 0);

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
    RTTR_Assert(state == CS_CONFIG);
    LOG.writeToFile("<<< NMS_GGS_CHANGE\n");

    gameLobby->GetSettings() = msg.ggs;

    if(ci)
        ci->CI_GGSChanged(msg.ggs);
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
    if(state != CS_LOADING && state != CS_GAME)
        return;
    if(msg.player >= gw->GetPlayerCount())
        return;
    // LOG.writeToFile("CLIENT <<< GC %u\n") % msg.player;
    // Nachricht in Queue einhängen
    gw->GetPlayer(msg.player).gc_queue.push(msg);
    // If this is our GC then it must be the next and only command as we need to execute this before we even send the next one
    RTTR_Assert(msg.player != playerId_ || gw->GetPlayer(msg.player).gc_queue.size() == 1);
}

void GameClient::IncreaseSpeed()
{
    if(framesinfo.gfLengthReq > 10)
        framesinfo.gfLengthReq -= 10;

#ifndef NDEBUG
    else if(framesinfo.gfLengthReq == 10)
        framesinfo.gfLengthReq = 1;
#endif

    else
        framesinfo.gfLengthReq = 70;

    send_queue.push(new GameMessage_Server_Speed(framesinfo.gfLengthReq));
}

void GameClient::DecreaseSpeed()
{
    if(framesinfo.gfLengthReq == 70)
    {
#ifndef NDEBUG
        framesinfo.gfLengthReq = 1;
#else
        framesinfo.gfLengthReq = 10;
#endif
    }
#ifndef NDEBUG
    else if(framesinfo.gfLengthReq == 1)
    {
        framesinfo.gfLengthReq = 10;
    }
#endif
    else
    {
        framesinfo.gfLengthReq += 10;
    }

    send_queue.push(new GameMessage_Server_Speed(framesinfo.gfLengthReq));
}

void GameClient::IncreaseReplaySpeed()
{
    if(replay_mode)
    {
        if(framesinfo.gf_length > 10)
            framesinfo.gf_length -= 10;
        else if(framesinfo.gf_length == 10)
            framesinfo.gf_length = 1;
    }
}

void GameClient::DecreaseReplaySpeed()
{
    if(replay_mode)
    {
        if(framesinfo.gf_length == 1)
            framesinfo.gf_length = 10;
        else if(framesinfo.gf_length < 1000)
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
            framesinfo.gfNrServer -=
              framesinfo.gfNrServer % framesinfo.nwf_length; // Set the value of the next NWF, not some GFs after that
        }
        RTTR_Assert(framesinfo.gf_length == msg.gf_length);
    } else
    {
        RTTR_Assert(framesinfo.gfNrServer == msg.nr); // We expect the next message when the server is at a NWF
        framesinfo.gfNrServer = msg.nr + framesinfo.nwf_length;
        framesinfo.gfNrServer -= framesinfo.gfNrServer % framesinfo.nwf_length; // Set the value of the next NWF, not some GFs after that
    }

    // LOG.writeToFile("framesinfo.gf_nr(%d) == framesinfo.gfNrServer(%d)\n") % framesinfo.gf_nr % framesinfo.gfNrServer;
}

/**
 *  NFC Pause-Nachricht von Server
 *
 *  @param[in] message Nachricht, welche ausgeführt wird
 */
void GameClient::OnGameMessage(const GameMessage_Pause& msg)
{
    if(framesinfo.isPaused == msg.paused)
        return;
    framesinfo.isPaused = msg.paused;

    LOG.writeToFile("<<< NMS_NFC_PAUSE(%1%)\n") % msg.paused;

    if(msg.paused)
        ci->CI_GamePaused();
    else
        ci->CI_GameResumed();
}

/**
 *  NFC GetAsyncLog von Server
 *
 *  @param[in] message Nachricht, welche ausgeführt wird
 */
void GameClient::OnGameMessage(const GameMessage_GetAsyncLog& /*msg*/)
{
    // AsyncLog an den Server senden

    std::vector<RandomEntry> async_log = RANDOM.GetAsyncLog();

    // stückeln...
    std::vector<RandomEntry> part;
    for(std::vector<RandomEntry>::iterator it = async_log.begin(); it != async_log.end(); ++it)
    {
        part.push_back(*it);

        if(part.size() == 10)
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
    RTTR_Assert(state == CS_GAME);
    bool is_lagging = false;

    for(unsigned i = 0; i < gw->GetPlayerCount(); ++i)
    {
        GamePlayer& player = gw->GetPlayer(i);
        if(player.isUsed())
        {
            if(player.gc_queue.empty())
            {
                player.is_lagging = true;
                is_lagging = true;
            } else
                player.is_lagging = false;
        }
    }

    return is_lagging;
}

/// Führt für alle Spieler einen Statistikschritt aus, wenn die Zeit es verlangt
void GameClient::StatisticStep()
{
    for(unsigned i = 0; i < gw->GetPlayerCount(); ++i)
        gw->GetPlayer(i).StatisticStep();

    // Check objective if there is one and there are at least two players
    if(GetGGS().objective != GO_CONQUER3_4 && GetGGS().objective != GO_TOTALDOMINATION)
        return;

    // check winning condition
    unsigned max = 0, sum = 0, best = 0xFFFF, maxteam = 0, bestteam = 0xFFFF;

    // Find out best player. Since at least 3/4 of the populated land is needed to win, we don't care about ties.
    for(unsigned i = 0; i < gw->GetPlayerCount(); ++i)
    {
        GamePlayer& player = gw->GetPlayer(i);
        if(GetGGS().lockedTeams) // in games with locked team settings check for team victory
        {
            if(player.IsDefeated())
                continue;
            unsigned curteam = 0;
            unsigned teampoints = 0;
            for(unsigned j = 0; j < gw->GetPlayerCount(); ++j)
            {
                if(i == j || !player.IsAlly(j))
                    continue;
                GamePlayer& teamPlayer = gw->GetPlayer(j);
                if(!teamPlayer.IsDefeated())
                {
                    curteam = curteam | (1 << j);
                    teampoints += teamPlayer.GetStatisticCurrentValue(STAT_COUNTRY);
                }
            }
            teampoints += player.GetStatisticCurrentValue(STAT_COUNTRY);
            curteam = curteam | (1 << i);
            if(teampoints > maxteam && teampoints > player.GetStatisticCurrentValue(STAT_COUNTRY))
            {
                maxteam = teampoints;
                bestteam = curteam;
            }
        }
        unsigned v = player.GetStatisticCurrentValue(STAT_COUNTRY);
        if(v > max)
        {
            max = v;
            best = i;
        }

        sum += v;
    }

    switch(GetGGS().objective)
    {
        case GO_CONQUER3_4: // at least 3/4 of the land
            if((max * 4 >= sum * 3) && (best != 0xFFFF))
            {
                ggs.objective = GO_NONE;
            }
            if((maxteam * 4 >= sum * 3) && (bestteam != 0xFFFF))
            {
                ggs.objective = GO_NONE;
            }
            break;

        case GO_TOTALDOMINATION: // whole populated land
            if((max == sum) && (best != 0xFFFF))
            {
                ggs.objective = GO_NONE;
            }
            if((maxteam == sum) && (bestteam != 0xFFFF))
            {
                ggs.objective = GO_NONE;
            }
            break;
        default: break;
    }

    // We have a winner! Objective was changed to GO_NONE to avoid further checks.
    if(GetGGS().objective == GO_NONE)
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
    const unsigned curGF = GetGFNumber();
    unsigned currentTime = VIDEODRIVER.GetTickCount();

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
    if(skipping || skiptogf > curGF || (currentTime - framesinfo.lastTime) >= framesinfo.gf_length)
    {
        if(replay_mode)
        {
            // In replay mode we have all commands in  the file -> Execute them
            ExecuteGameFrame_Replay();
        } else
        {
            // Is it time for a NWF, handle that first
            if(curGF % framesinfo.nwf_length == 0)
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
                if(framesinfo.gfNrServer < curGF)
                    return;
                RTTR_Assert(framesinfo.gfNrServer <= curGF + framesinfo.nwf_length);

                ExecuteNWF();

                RTTR_Assert(framesinfo.gfLenghtNew != 0);
                if(framesinfo.gfLenghtNew != framesinfo.gf_length)
                {
                    unsigned oldGfLen = framesinfo.gf_length;
                    int oldNwfLen = framesinfo.nwf_length;
                    framesinfo.ApplyNewGFLength();
                    framesinfo.gfLengthReq = framesinfo.gf_length;

                    // Adjust next confirmation for next NWF (if we have it already)
                    if(framesinfo.gfNrServer != curGF)
                    {
                        RTTR_Assert(framesinfo.gfNrServer == curGF + oldNwfLen);
                        framesinfo.gfNrServer = framesinfo.gfNrServer - oldNwfLen + framesinfo.nwf_length;
                        // Make it a NWF (mostly for validation and consistency)
                        framesinfo.gfNrServer -= framesinfo.gfNrServer % framesinfo.nwf_length;
                    }

                    LOG.write("Client: %u/%u: Speed changed from %u to %u (NWF: %u to %u)\n") % framesinfo.gfNrServer % curGF % oldGfLen
                      % framesinfo.gf_length % oldNwfLen % framesinfo.nwf_length;
                }
                // "pop" the length
                framesinfo.gfLenghtNew = framesinfo.gfLenghtNew2;
                framesinfo.gfLenghtNew2 = 0;
            }

            NextGF();
        }

        RTTR_Assert(replay_mode || curGF <= framesinfo.gfNrServer + framesinfo.nwf_length);
        // Store this timestamp
        framesinfo.lastTime = currentTime;
        // Reset frameTime
        framesinfo.frameTime = 0;

        HandleAutosave();

        // GF-Ende im Replay aktualisieren
        if(!replay_mode)
            replayinfo.replay.UpdateLastGF(curGF);
    } else
    {
        // Next GF not yet reached, just update the time in the current one for drawing
        framesinfo.frameTime = currentTime - framesinfo.lastTime;
        RTTR_Assert(framesinfo.frameTime < framesinfo.gf_length);
    }
}

void GameClient::HandleAutosave()
{
    // If inactive or during replay -> no autosave
    if(!SETTINGS.interface.autosave_interval || replay_mode)
        return;

    // Alle .... GF
    if(GetGFNumber() % SETTINGS.interface.autosave_interval == 0)
    {
        std::string tmp = GetFilePath(FILE_PATHS[85]);

        if(this->mapinfo.title.length())
        {
            tmp += this->mapinfo.title;
            tmp += " (";
            tmp += _("Auto-Save");
            tmp += ").sav";
        } else
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
    // Update statistic every 750 GFs (30 seconds on 'fast')
    if(GetGFNumber() % 750 == 0)
        StatisticStep();
    //  EventManager Bescheid sagen
    em->ExecuteNextGF();
    // Notfallprogramm durchlaufen lassen
    for(unsigned i = 0; i < gw->GetPlayerCount(); ++i)
    {
        GamePlayer& player = gw->GetPlayer(i);
        if(player.isUsed())
        {
            // Auf Notfall testen (Wenige Bretter/Steine und keine Holzindustrie)
            player.TestForEmergencyProgramm();
            // Bündnisse auf Aktualität überprüfen
            player.TestPacts();
        }
    }

    if(human_ai)
    {
        human_ai->RunGF(GetGFNumber(), (GetGFNumber() % framesinfo.nwf_length == 0));

        const std::vector<gc::GameCommandPtr>& ai_gcs = human_ai->GetGameCommands();
        gameCommands_.insert(gameCommands_.end(), ai_gcs.begin(), ai_gcs.end());
        human_ai->FetchGameCommands();
    }

    if(gw->HasLua())
        gw->GetLua().EventGameFrame(GetGFNumber());
}

void GameClient::ExecuteAllGCs(const GameMessage_GameCommand& gcs)
{
    for(unsigned char i = 0; i < gcs.gcs.size(); ++i)
    {
        // NC ausführen
        gcs.gcs[i]->Execute(*gw, gcs.player);
    }
}

/**
 *  Sendet ein NC-Paket ohne Befehle.
 */
void GameClient::SendNothingNC(int checksum)
{
    if(checksum == -1)
        checksum = RANDOM.GetChecksum();

    send_queue.push(new GameMessage_GameCommand(playerId_, AsyncChecksum(checksum), std::vector<gc::GameCommandPtr>()));
}

void GameClient::WritePlayerInfo(SavedFile& file)
{
    RTTR_Assert(state == CS_LOADING || state == CS_GAME);
    // Spielerdaten
    for(unsigned i = 0; i < gw->GetPlayerCount(); ++i)
        file.AddPlayer(gw->GetPlayer(i));
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
    replayinfo.replay.ggs = GetGGS();

    // Mapname
    replayinfo.replay.mapName = mapinfo.title;
    replayinfo.replay.mapFileName = bfs::path(mapinfo.filepath).filename().string();

    // Datei speichern
    if(!replayinfo.replay.WriteHeader(fileName, mapinfo))
        LOG.write("GameClient::WriteReplayHeader: WARNING: File couldn't be opened. Don't use a replayinfo.replay.\n");
}

bool GameClient::StartReplay(const std::string& path)
{
    RTTR_Assert(state == CS_STOPPED);
    replayinfo.Clear();
    mapinfo.Clear();
    replayinfo.filename = path;

    if(!replayinfo.replay.LoadHeader(path, &mapinfo))
    {
        LOG.write(_("Invalid Replay %1%! Reason: %2%\n")) % path
          % (replayinfo.replay.GetLastErrorMsg().empty() ? _("Unknown") : replayinfo.replay.GetLastErrorMsg());
        if(ci)
            ci->CI_Error(CE_WRONGMAP);
        return false;
    }

    gameLobby.reset(new GameLobby(replayinfo.replay.GetPlayerCount()));

    // NWF-Länge
    framesinfo.nwf_length = replayinfo.replay.nwf_length;

    // Spielerdaten
    for(unsigned i = 0; i < replayinfo.replay.GetPlayerCount(); ++i)
        gameLobby->GetPlayer(i) = JoinPlayerInfo(replayinfo.replay.GetPlayer(i));

    bool playerFound = false;
    // Find a player to spectate from
    // First find a human player
    for(unsigned char i = 0; i < gameLobby->GetPlayerCount(); ++i)
    {
        if(gameLobby->GetPlayer(i).ps == PS_OCCUPIED)
        {
            playerId_ = i;
            playerFound = true;
            break;
        }
    }
    if(!playerFound)
    {
        // If no human found, take the first AI
        for(unsigned char i = 0; i < gameLobby->GetPlayerCount(); ++i)
        {
            if(gameLobby->GetPlayer(i).ps == PS_AI)
            {
                playerId_ = i;
                break;
            }
        }
    }

    // GGS-Daten
    gameLobby->GetSettings() = replayinfo.replay.ggs;

    switch(mapinfo.type)
    {
        default: break;
        case MAPTYPE_OLDMAP:
        {
            // Richtigen Pfad zur Map erstellen
            mapinfo.filepath = GetFilePath(FILE_PATHS[48]) + replayinfo.replay.mapFileName;
            if(!mapinfo.mapData.DecompressToFile(mapinfo.filepath))
            {
                LOG.write(_("Error decompressing map file"));
                if(ci)
                    ci->CI_Error(CE_WRONGMAP);
                Stop();
                return false;
            }
            if(mapinfo.luaData.length)
            {
                mapinfo.luaFilepath = mapinfo.filepath.substr(0, mapinfo.filepath.length() - 3) + "lua";
                if(!mapinfo.luaData.DecompressToFile(mapinfo.luaFilepath))
                {
                    LOG.write(_("Error decompressing lua file"));
                    if(ci)
                        ci->CI_Error(CE_WRONGMAP);
                    Stop();
                    return false;
                }
            }
        }
        break;
        case MAPTYPE_SAVEGAME: break;
    }

    replay_mode = true;
    replayinfo.async = 0;
    replayinfo.end = false;

    try
    {
        StartGame(replayinfo.replay.random_init);
    } catch(SerializedGameData::Error& error)
    {
        LOG.write(_("Error when loading game from replay: %s\n")) % error.what();
        if(ci)
            ci->CI_Error(CE_WRONGMAP);
        Stop();
        return false;
    }

    replayinfo.replay.ReadGF(&replayinfo.next_gf);

    return true;
}

unsigned GameClient::GetGlobalAnimation(const unsigned short max, const unsigned char factor_numerator,
                                        const unsigned char factor_denumerator, const unsigned offset)
{
    // Unit for animations is 630ms (dividable by 2, 3, 5, 6, 7, 10, 15, ...)
    // But this also means: If framerate drops below approx. 15Hz, you won't see
    // every frame of an 8-part animation anymore.
    // An animation runs fully in (factor_numerator / factor_denumerator) multiples of 630ms
    const unsigned unit = 630 /*ms*/ * factor_numerator / factor_denumerator;
    // Good approximation of current time in ms
    // (Accuracy of a possibly expensive VideoDriverWrapper::GetTicks() isn't needed here):
    const unsigned currenttime = framesinfo.lastTime + framesinfo.frameTime;
    return ((currenttime % unit) * max / unit + offset) % max;
}

unsigned GameClient::Interpolate(unsigned max_val, GameEvent* ev)
{
    RTTR_Assert(ev);
    unsigned elapsedTime = (GetGFNumber() - ev->startGF) * framesinfo.gf_length + framesinfo.frameTime;
    unsigned duration = ev->length * framesinfo.gf_length;
    unsigned result = (max_val * elapsedTime) / duration;
    if(result >= max_val)
        RTTR_Assert(result < max_val);
    return result;
}

int GameClient::Interpolate(int x1, int x2, GameEvent* ev)
{
    RTTR_Assert(ev);
    unsigned elapsedTime = (GetGFNumber() - ev->startGF) * framesinfo.gf_length + framesinfo.frameTime;
    unsigned duration = ev->length * framesinfo.gf_length;
    return x1 + ((x2 - x1) * int(elapsedTime)) / int(duration);
}

void GameClient::ServerLost()
{
    if(ci)
        ci->CI_Error(CE_CONNECTIONLOST);

    Stop();
}

/**
 *  überspringt eine bestimmte Anzahl von Gameframes.
 *
 *  @param[in] dest_gf Zielgameframe
 */
void GameClient::SkipGF(unsigned gf, GameWorldView& gwv)
{
    if(gf <= GetGFNumber())
        return;

    unsigned start_ticks = VIDEODRIVER.GetTickCount();

    if(!replay_mode)
    {
        // unpause before skipping
        GAMESERVER.SetPaused(false);
        GAMESERVER.skiptogf = gf;
        skiptogf = gf;
        LOG.write("jumping from gf %i to gf %i \n") % GetGFNumber() % gf;
        return;
    }

    SetReplayPause(false);

    // GFs überspringen
    for(unsigned i = GetGFNumber(); i < gf; ++i)
    {
        if(i % 1000 == 0)
        {
            RoadBuildState road;
            road.mode = RM_DISABLED;

            // spiel aktualisieren
            gwv.Draw(road);

            // text oben noch hinschreiben
            char nwf_string[256];
            snprintf(nwf_string, 255, _("current GF: %u - still fast forwarding: %d GFs left (%d %%)"), GetGFNumber(), gf - i,
                     (i * 100 / gf));
            LargeFont->Draw(DrawPoint(VIDEODRIVER.GetScreenSize() / 2u), nwf_string, glArchivItem_Font::DF_CENTER, 0xFFFFFF00);

            VIDEODRIVER.SwapBuffers();
        }
        ExecuteGameFrame(true);
        // LOG.write(("jumping: now at gf %i\n", framesinfo.nr);
    }

    // Spiel pausieren & text ausgabe wie lang das jetzt gedauert hat
    unsigned ticks = VIDEODRIVER.GetTickCount() - start_ticks;
    char text[256];
    snprintf(text, sizeof(text), _("Jump finished (%.3f seconds)."), (double)ticks / 1000.0);
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
    Point<int> moonPos = VIDEODRIVER.GetMousePos();
    moonPos.y -= 40;
    LOADER.GetImageN("resource", 33)->DrawFull(moonPos);
    VIDEODRIVER.SwapBuffers();

    Savegame save;

    // Timestamp der Aufzeichnung
    save.save_time = TIME.CurrentTime();
    // Mapname
    save.mapName = this->mapinfo.title;

    WritePlayerInfo(save);

    // GGS-Daten
    save.ggs = GetGGS();

    save.start_gf = GetGFNumber();

    // Enable/Disable debugging of savegames
    save.sgd.debugMode = SETTINGS.global.debugMode;

    // Spiel serialisieren
    save.sgd.MakeSnapshot(*gw);

    // Und alles speichern
    if(!save.Save(filename))
        return 1;
    else
        return 0;
}

void GameClient::ResetVisualSettings()
{
    GetPlayer(playerId_).FillVisualSettings(visual_settings);
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
    if(framesinfo.isPaused || GetPlayer(playerId_).IsDefeated() || IsReplayModeOn())
    {
        delete gc;
        return false;
    }

    gameCommands_.push_back(gc);
    return true;
}

unsigned GameClient::GetPlayerCount() const
{
    RTTR_Assert(state == CS_LOADING || state == CS_GAME);
    return gw->GetPlayerCount();
}

GamePlayer& GameClient::GetPlayer(const unsigned id)
{
    RTTR_Assert(state == CS_LOADING || state == CS_GAME);
    RTTR_Assert(id < GetPlayerCount());
    return gw->GetPlayer(id);
}

bool GameClient::IsSinglePlayer() const
{
    RTTR_Assert(state == CS_LOADING || state == CS_GAME);
    return gw->IsSinglePlayer();
}

/// Erzeugt einen KI-Player, der mit den Daten vom GameClient gefüttert werden muss (zusätzlich noch mit den GameServer)
AIBase* GameClient::CreateAIPlayer(unsigned playerId, const AI::Info& aiInfo)
{
    if(aiInfo.type == AI::DEFAULT)
        return new AIPlayerJH(playerId, *gw, aiInfo.level);
    else
        return new AIPlayer(playerId, *gw, aiInfo.level);
}

const GlobalGameSettings& GameClient::GetGGS() const
{
    RTTR_Assert(state == CS_LOADING || state == CS_GAME);
    return ggs;
}

/// Wandelt eine GF-Angabe in eine Zeitangabe um (HH:MM:SS oder MM:SS wenn Stunden = 0)
std::string GameClient::FormatGFTime(const unsigned gf) const
{
    // In Sekunden umrechnen
    unsigned seconds = gf * framesinfo.gf_length / 1000;

    // Angaben rausfiltern
    unsigned hours = seconds / 3600;
    seconds -= hours * 3600;
    unsigned minutes = seconds / 60;
    seconds -= minutes * 60;

    char str[64];

    // ganze Stunden mit dabei? Dann entsprechend anderes format, ansonsten ignorieren wir die einfach
    if(hours)
        sprintf(str, "%02u:%02u:%02u", hours, minutes, seconds);
    else
        sprintf(str, "%02u:%02u", minutes, seconds);

    return std::string(str);
}

/// Is tournament mode activated (0 if not)? Returns the durations of the tournament mode in gf otherwise
unsigned GameClient::GetTournamentModeDuration() const
{
    if(unsigned(GetGGS().objective) >= OBJECTIVES_COUNT)
        return TOURNAMENT_MODES_DURATION[GetGGS().objective - OBJECTIVES_COUNT] * 60 * 1000 / framesinfo.gf_length;
    else
        return 0;
}

void GameClient::ToggleHumanAIPlayer()
{
    RTTR_Assert(!IsReplayModeOn());
    if(human_ai)
        human_ai.reset();
    else
        human_ai.reset(CreateAIPlayer(playerId_, AI::Info(AI::DEFAULT, AI::EASY)));
}

void GameClient::RequestSwapToPlayer(const unsigned char newId)
{
    if(state != CS_GAME)
        return;
    GamePlayer& player = GetPlayer(newId);
    if(player.ps == PS_AI && player.aiInfo.type == AI::DUMMY)
        send_queue.push(new GameMessage_Player_Swap(playerId_, newId));
}

bool GameClient::IsLagging(const unsigned id)
{
    return GetPlayer(id).is_lagging;
}
