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
#include "GameClient.h"
#include "ClientPlayer.h"
#include "ClientPlayers.h"
#include "EventManager.h"
#include "Game.h"
#include "GameEvent.h"
#include "GameInterface.h"
#include "GameLobby.h"
#include "GameManager.h"
#include "GameMessage_GameCommand.h"
#include "GameObject.h"
#include "GlobalVars.h"
#include "JoinPlayerInfo.h"
#include "Loader.h"
#include "PlayerGameCommands.h"
#include "RTTR_Version.h"
#include "ReplayInfo.h"
#include "RttrConfig.h"
#include "Savegame.h"
#include "SerializedGameData.h"
#include "Settings.h"
#include "addons/const_addons.h"
#include "ai/AIPlayer.h"
#include "drivers/VideoDriverWrapper.h"
#include "factories/AIFactory.h"
#include "files.h"
#include "helpers/Deleter.h"
#include "network/ClientInterface.h"
#include "network/GameMessages.h"
#include "network/GameServer.h"
#include "ogl/glArchivItem_Font.h"
#include "ogl/glArchivItem_Map.h"
#include "postSystem/PostManager.h"
#include "random/Random.h"
#include "world/GameWorld.h"
#include "world/GameWorldView.h"
#include "gameTypes/RoadBuildState.h"
#include "gameData/GameConsts.h"
#include "libsiedler2/ArchivItem_Map_Header.h"
#include "libsiedler2/prototypen.h"
#include "libutil/SocketSet.h"
#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <boost/interprocess/smart_ptr/unique_ptr.hpp>
#include <boost/smart_ptr/scoped_array.hpp>
#include <cerrno>
#include <iostream>

void GameClient::ClientConfig::Clear()
{
    server.clear();
    gameName.clear();
    password.clear();
    port = 0;
    isHost = false;
}

GameClient::GameClient() : skiptogf(0), mainPlayer(0), state(CS_STOPPED), ci(NULL), replayMode(false)
{
    clientconfig.Clear();
    framesinfo.Clear();
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
    if(!mainPlayer.socket.Connect(server, port, use_ipv6, SETTINGS.proxy))
    {
        LOG.write("GameClient::Connect: ERROR: Connect failed!\n");
        return false;
    }

    state = CS_CONNECT;

    if(ci)
        ci->CI_NextConnectState(CS_WAITFORANSWER);

    // Es wird kein Replay abgespielt, sondern dies ist ein richtiges Spiel
    replayMode = false;

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
    set.Add(mainPlayer.socket);
    if(set.Select(0, 0) > 0)
    {
        // nachricht empfangen
        if(!mainPlayer.receiveMsgs())
        {
            LOG.write("Receiving Message from server failed\n");
            ServerLost();
        }
    }

    // nun auf Fehler prüfen
    set.Clear();

    // zum set hinzufügen
    set.Add(mainPlayer.socket);

    // auf fehler prüfen
    if(set.Select(0, 2) > 0)
    {
        if(set.InSet(mainPlayer.socket))
        {
            // Server ist weg
            LOG.write("Error on socket to server\n");
            ServerLost();
        }
    }

    if(state == CS_GAME)
        ExecuteGameFrame();

    // maximal 10 Pakete verschicken
    mainPlayer.sendMsgs(10);

    mainPlayer.executeMsgs(*this, false);
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

    if(replayinfo)
    {
        replayinfo->replay.StopRecording();
        replayinfo.reset();
    }

    mainPlayer.closeConnection(false);

    // clear jump target
    skiptogf = 0;

    // Consistency check: No game, no lobby remaining
    RTTR_Assert(!game);
    RTTR_Assert(!gameLobby);

    state = CS_STOPPED;
    LOG.write("client state changed to stop\n");
}

boost::shared_ptr<GameLobby> GameClient::GetGameLobby()
{
    RTTR_Assert(state == CS_CONFIG);
    RTTR_Assert(gameLobby);
    return gameLobby;
}

/**
 *  Startet ein Spiel oder Replay.
 *
 *  @param[in] random_init Initialwert des Zufallsgenerators.
 */
void GameClient::StartGame(const unsigned random_init)
{
    RTTR_Assert(state == CS_CONFIG || (state == CS_STOPPED && replayMode));

    // Mond malen
    Point<int> moonPos = VIDEODRIVER.GetMousePos();
    moonPos.y -= 40;
    LOADER.GetImageN("resource", 33)->DrawFull(moonPos);
    VIDEODRIVER.SwapBuffers();

    // Start in pause mode
    framesinfo.isPaused = true;

    // Je nach Geschwindigkeit GF-Länge einstellen
    framesinfo.gf_length = SPEED_GF_LENGTHS[gameLobby->getSettings().speed];
    framesinfo.gfLengthReq = framesinfo.gf_length;

    // Random-Generator initialisieren
    RANDOM.Init(random_init);

    if(!IsReplayModeOn() && mapinfo.savegame && !mapinfo.savegame->Load(mapinfo.filepath, true, true))
    {
        if(ci)
            ci->CI_Error(CE_WRONGMAP);
        Stop();
        return;
    }

    // If we have a savegame, start at its first GF, else at 0
    unsigned startGF = (mapinfo.type == MAPTYPE_SAVEGAME) ? mapinfo.savegame->start_gf : 0;
    // Create the game
    game.reset(
      new Game(gameLobby->getSettings(), startGF, std::vector<PlayerInfo>(gameLobby->getPlayers().begin(), gameLobby->getPlayers().end())));
    clientPlayers.reset(new ClientPlayers());
    for(unsigned id = 0; id < gameLobby->getNumPlayers(); id++)
    {
        if(gameLobby->getPlayer(id).isUsed())
            clientPlayers->add(id);
    }
    // Release lobby
    gameLobby.reset();

    state = CS_LOADING;

    if(ci)
        ci->CI_GameStarted(game);

    // Get standard settings before they get overwritten
    GetPlayer(mainPlayer.playerId).FillVisualSettings(default_settings);

    GameWorld& gameWorld = game->world;
    if(mapinfo.savegame)
        mapinfo.savegame->sgd.ReadSnapshot(gameWorld);
    else
    {
        RTTR_Assert(mapinfo.type != MAPTYPE_SAVEGAME);
        /// Startbündnisse setzen
        for(unsigned i = 0; i < gameWorld.GetNumPlayers(); ++i)
            gameWorld.GetPlayer(i).MakeStartPacts();

        gameWorld.LoadMap(mapinfo.filepath, mapinfo.luaFilepath);

        /// Evtl. Goldvorkommen ändern
        Resource::Type target; // löschen
        switch(game->ggs.getSelection(AddonId::CHANGE_GOLD_DEPOSITS))
        {
            case 0:
            default: target = Resource::Gold; break;
            case 1: target = Resource::Nothing; break;
            case 2: target = Resource::Iron; break;
            case 3: target = Resource::Coal; break;
            case 4: target = Resource::Granite; break;
        }
        gameWorld.ConvertMineResourceTypes(Resource::Gold, target);

        if (game->ggs.getSelection(AddonId::EXHAUSTIBLE_WATER) == 1)         // Water everywhere is enabled.
        {
            gameWorld.FillWaterEverywhere();
        }
    }
    gameWorld.InitAfterLoad();

    // Update visual settings
    ResetVisualSettings();

    // Zeit setzen
    framesinfo.lastTime = VIDEODRIVER.GetTickCount();

    if(!replayMode)
    {
        RTTR_Assert(!replayinfo);
        StartReplayRecording(random_init);
    }

    // Daten nach dem Schreiben des Replays ggf wieder löschen
    mapinfo.mapData.Clear();
}

void GameClient::GameStarted()
{
    RTTR_Assert(state == CS_LOADING);
    state = CS_GAME; // zu gamestate wechseln

    framesinfo.isPaused = replayMode;

    // Send empty GC for first NWF
    if(!replayMode)
        SendNothingNC();

    GAMEMANAGER.ResetAverageFPS();
    game->Start(mapinfo.savegame);
}

void GameClient::ExitGame()
{
    RTTR_Assert(state == CS_GAME || state == CS_LOADING);
    // Spielwelt zerstören
    human_ai.reset();
    game.reset();
    clientPlayers.reset();
    // Clear remaining commands
    gameCommands_.clear();
}

unsigned GameClient::GetGFNumber() const
{
    return game->em->GetCurrentGF();
}

/**
 *  Ping-Nachricht.
 */
bool GameClient::OnGameMessage(const GameMessage_Ping& /*msg*/)
{
    mainPlayer.sendMsgAsync(new GameMessage_Pong(0xFF));
    return true;
}

/**
 *  Player-ID-Nachricht.
 */
bool GameClient::OnGameMessage(const GameMessage_Player_Id& msg)
{
    // haben wir eine ungültige ID erhalten? (aka Server-Voll)
    if(msg.playerId == 0xFFFFFFFF)
    {
        if(ci)
            ci->CI_Error(CE_SERVERFULL);

        Stop();
        return true;
    }

    mainPlayer.playerId = msg.playerId;

    // Server-Typ senden
    mainPlayer.sendMsgAsync(new GameMessage_Server_Type(clientconfig.servertyp, RTTR_Version::GetRevision()));
    return true;
}

/**
 *  Player-List-Nachricht.
 */
bool GameClient::OnGameMessage(const GameMessage_Player_List& msg)
{
    RTTR_Assert(state == CS_CONNECT);
    RTTR_Assert(gameLobby);
    RTTR_Assert(gameLobby->getNumPlayers() == msg.playerInfos.size());

    for(unsigned i = 0; i < gameLobby->getNumPlayers(); ++i)
        gameLobby->getPlayer(i) = msg.playerInfos[i];

    state = CS_CONFIG;
    if(ci)
        ci->CI_NextConnectState(CS_FINISHED);
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// player joined
/// @param message  Nachricht, welche ausgeführt wird
bool GameClient::OnGameMessage(const GameMessage_Player_New& msg)
{
    LOG.writeToFile("<<< NMS_PLAYER_NEW(%d)\n") % unsigned(msg.player);
    RTTR_Assert(state == CS_CONFIG);
    if(msg.player >= gameLobby->getNumPlayers())
        return true;

    JoinPlayerInfo& playerInfo = gameLobby->getPlayer(msg.player);

    playerInfo.name = msg.name;
    playerInfo.ps = PS_OCCUPIED;
    playerInfo.ping = 0;
    playerInfo.InitRating();

    if(ci)
        ci->CI_NewPlayer(msg.player);
    return true;
}

bool GameClient::OnGameMessage(const GameMessage_Player_Ping& msg)
{
    if(state == CS_CONFIG)
    {
        if(msg.player >= gameLobby->getNumPlayers())
            return true;
        gameLobby->getPlayer(msg.player).ping = msg.ping;
    } else if(state == CS_LOADING || state == CS_GAME)
    {
        if(msg.player >= GetNumPlayers())
            return true;
        GetPlayer(msg.player).ping = msg.ping;
    } else
    {
        RTTR_Assert(false);
        return true;
    }

    if(ci)
        ci->CI_PingChanged(msg.player, msg.ping);
    return true;
}

/**
 *  Player-Toggle-State-Nachricht.
 */
bool GameClient::OnGameMessage(const GameMessage_Player_Set_State& msg)
{
    RTTR_Assert(state == CS_CONFIG);
    if(msg.player >= gameLobby->getNumPlayers())
        return true;

    JoinPlayerInfo& playerInfo = gameLobby->getPlayer(msg.player);
    playerInfo.ps = msg.ps;
    playerInfo.aiInfo = msg.aiInfo;
    playerInfo.InitRating();
    if(playerInfo.ps == PS_AI)
        playerInfo.SetAIName(msg.player);

    if(ci)
        ci->CI_PSChanged(msg.player, playerInfo.ps);
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// nation button gedrückt
/// @param message  Nachricht, welche ausgeführt wird
bool GameClient::OnGameMessage(const GameMessage_Player_Set_Nation& msg)
{
    RTTR_Assert(state == CS_CONFIG);
    if(msg.player >= gameLobby->getNumPlayers())
        return true;

    gameLobby->getPlayer(msg.player).nation = msg.nation;

    if(ci)
        ci->CI_NationChanged(msg.player, msg.nation);
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// team button gedrückt
/// @param message  Nachricht, welche ausgeführt wird
bool GameClient::OnGameMessage(const GameMessage_Player_Set_Team& msg)
{
    RTTR_Assert(state == CS_CONFIG);
    if(msg.player >= gameLobby->getNumPlayers())
        return true;

    gameLobby->getPlayer(msg.player).team = msg.team;

    if(ci)
        ci->CI_TeamChanged(msg.player, msg.team);
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// color button gedrückt
/// @param message  Nachricht, welche ausgeführt wird
bool GameClient::OnGameMessage(const GameMessage_Player_Set_Color& msg)
{
    RTTR_Assert(state == CS_CONFIG);
    if(msg.player >= gameLobby->getNumPlayers())
        return true;

    gameLobby->getPlayer(msg.player).color = msg.color;

    if(ci)
        ci->CI_ColorChanged(msg.player, msg.color);
    return true;
}

/**
 *  Ready-state eines Spielers hat sich geändert.
 *
 *  @param[in] message Nachricht, welche ausgeführt wird
 */
bool GameClient::OnGameMessage(const GameMessage_Player_Ready& msg)
{
    LOG.writeToFile("<<< NMS_PLAYER_READY(%d, %s)\n") % unsigned(msg.player) % (msg.ready ? "true" : "false");

    RTTR_Assert(state == CS_CONFIG);
    if(msg.player >= gameLobby->getNumPlayers())
        return true;

    gameLobby->getPlayer(msg.player).isReady = msg.ready;

    if(ci)
        ci->CI_ReadyChanged(msg.player, msg.ready);
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// player gekickt
/// @param message  Nachricht, welche ausgeführt wird
bool GameClient::OnGameMessage(const GameMessage_Player_Kicked& msg)
{
    LOG.writeToFile("<<< NMS_PLAYER_KICKED(%d, %d, %d)\n") % unsigned(msg.player) % unsigned(msg.cause) % msg.param;

    RTTR_Assert(state == CS_CONFIG || state == CS_LOADING || state == CS_GAME);

    if(state == CS_CONFIG)
    {
        if(msg.player >= gameLobby->getNumPlayers())
            return true;
        gameLobby->getPlayer(msg.player).ps = PS_FREE;
    } else
    {
        // Im Spiel anzeigen, dass der Spieler das Spiel verlassen hat
        game->world.GetPlayer(msg.player).ps = PS_AI;
    }

    if(ci)
        ci->CI_PlayerLeft(msg.player);
    return true;
}

bool GameClient::OnGameMessage(const GameMessage_Player_Swap& msg)
{
    LOG.writeToFile("<<< NMS_PLAYER_SWAP(%u, %u)\n") % unsigned(msg.player) % unsigned(msg.player2);

    RTTR_Assert(state == CS_CONFIG || state == CS_LOADING || state == CS_GAME);

    if(state == CS_CONFIG)
    {
        if(msg.player >= gameLobby->getNumPlayers() || msg.player2 >= gameLobby->getNumPlayers())
            return true;

        // During preparation just swap the players
        using std::swap;
        swap(gameLobby->getPlayer(msg.player), gameLobby->getPlayer(msg.player2));
        // Some things cannot be changed in savegames
        if(mapinfo.type == MAPTYPE_SAVEGAME)
            gameLobby->getPlayer(msg.player).FixSwappedSaveSlot(gameLobby->getPlayer(msg.player2));

        // Evtl. sind wir betroffen?
        if(mainPlayer.playerId == msg.player)
            mainPlayer.playerId = msg.player2;
        else if(mainPlayer.playerId == msg.player2)
            mainPlayer.playerId = msg.player;

        if(ci)
            ci->CI_PlayersSwapped(msg.player, msg.player2);
    } else
    {
        if(msg.player >= game->world.GetNumPlayers() || msg.player2 >= game->world.GetNumPlayers())
            return true;
        ChangePlayerIngame(msg.player, msg.player2);
    }
    return true;
}

/**
 *  Server-Typ-Nachricht.
 */
bool GameClient::OnGameMessage(const GameMessage_Server_TypeOK& msg)
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
            return true;
        }
        break;

        case 2:
        {
            if(ci)
                ci->CI_Error(CE_WRONGVERSION);
            Stop();
            return true;
        }
        break;
    }

    mainPlayer.sendMsgAsync(new GameMessage_Server_Password(clientconfig.password));

    if(ci)
        ci->CI_NextConnectState(CS_QUERYPW);
    return true;
}

/**
 *  Server-Passwort-Nachricht.
 */
bool GameClient::OnGameMessage(const GameMessage_Server_Password& msg)
{
    if(msg.password != "true")
    {
        if(ci)
            ci->CI_Error(CE_WRONGPW);

        Stop();
        return true;
    }

    mainPlayer.sendMsgAsync(new GameMessage_Player_Name(SETTINGS.lobby.name));

    if(ci)
        ci->CI_NextConnectState(CS_QUERYMAPNAME);
    return true;
}

/**
 *  Server-Name-Nachricht.
 */
bool GameClient::OnGameMessage(const GameMessage_Server_Name& msg)
{
    clientconfig.gameName = msg.name;

    if(ci)
        ci->CI_NextConnectState(CS_QUERYPLAYERLIST);
    return true;
}

/**
 *  Server-Start-Nachricht
 */
bool GameClient::OnGameMessage(const GameMessage_Server_Start& msg)
{
    // NWF-Länge bekommen wir vom Server
    framesinfo.nwf_length = msg.nwf_length;

    /// Beim Host muss das Spiel nicht nochmal gestartet werden, das hat der Server schon erledigt
    if(IsHost())
        return true;

    try
    {
        StartGame(msg.random_init);
    } catch(SerializedGameData::Error& error)
    {
        LOG.write("Error when loading game: %s\n") % error.what();
        GAMEMANAGER.ShowMenu();
        ExitGame();
    }
    return true;
}

/**
 *  Server-Chat-Nachricht.
 */
bool GameClient::OnGameMessage(const GameMessage_Server_Chat& msg)
{
    if(state == CS_GAME)
    {
        // Ingame message: Do some checking and logging
        if(msg.player >= game->world.GetNumPlayers())
            return true;

        /// Mit im Replay aufzeichnen
        replayinfo->replay.AddChatCommand(GetGFNumber(), msg.player, msg.destination, msg.text);

        GamePlayer& player = game->world.GetPlayer(msg.player);

        // Besiegte dürfen nicht mehr heimlich mit Verbüdeten oder Feinden reden
        if(player.IsDefeated() && msg.destination != CD_ALL)
            return true;
        // Entscheiden, ob ich ein Gegner oder Vebündeter bin vom Absender
        bool ally = player.IsAlly(mainPlayer.playerId);

        // Chatziel unerscheiden und ggf. nicht senden
        if(!ally && msg.destination == CD_ALLIES)
            return true;
        if(ally && msg.destination == CD_ENEMIES && msg.player != mainPlayer.playerId)
            return true;
    } else if(state == CS_CONFIG)
    {
        // GameLobby message: Just check for valid player
        if(msg.player >= gameLobby->getNumPlayers())
            return true;
    } else
        return true;

    if(ci)
        ci->CI_Chat(msg.player, msg.destination, msg.text);
    return true;
}

bool GameClient::OnGameMessage(const GameMessage_System_Chat& msg)
{
    SystemChat(msg.text, msg.player);
    return true;
}

/**
 *  Server-Async-Nachricht.
 */
bool GameClient::OnGameMessage(const GameMessage_Server_Async& msg)
{
    if(state != CS_GAME)
        return true;

    // Liste mit Namen und Checksummen erzeugen
    std::stringstream checksum_list;
    for(unsigned i = 0; i < msg.checksums.size(); ++i)
    {
        checksum_list << game->world.GetPlayer(i).name << ": " << msg.checksums[i];
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

    std::string timeStr = s25util::Time::FormatTime("async_%Y-%m-%d_%H-%i-%s");
    std::string filePathSave = RTTRCONFIG.ExpandPath(FILE_PATHS[85]) + "/" + timeStr + ".sav";
    std::string filePathLog = RTTRCONFIG.ExpandPath(FILE_PATHS[47]) + "/" + timeStr + "Player.log";
    RANDOM.SaveLog(filePathLog);
    SaveToFile(filePathSave);
    LOG.write("Async log saved at \"%s\", game saved at \"%s\"\n") % filePathLog % filePathSave;
    return true;
}

/**
 *  Server-Countdown-Nachricht.
 */
bool GameClient::OnGameMessage(const GameMessage_Server_Countdown& msg)
{
    if(ci)
        ci->CI_Countdown(msg.countdown);
    return true;
}

/**
 *  Server-Cancel-Countdown-Nachricht.
 */
bool GameClient::OnGameMessage(const GameMessage_Server_CancelCountdown& /*msg*/)
{
    if(ci)
        ci->CI_CancelCountdown();
    return true;
}

/**
 *  verarbeitet die MapInfo-Nachricht, in der die gepackte Größe,
 *  die normale Größe und Teilanzahl der Karte übertragen wird.
 *
 *  @param message Nachricht, welche ausgeführt wird
 */
bool GameClient::OnGameMessage(const GameMessage_Map_Info& msg)
{
    // full path
    mapinfo.filepath = RTTRCONFIG.ExpandPath(FILE_PATHS[48]) + "/" + msg.map_name;
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
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// Kartendaten
/// @param message  Nachricht, welche ausgeführt wird
bool GameClient::OnGameMessage(const GameMessage_Map_Data& msg)
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
            return true;
        }
        if(!mapinfo.luaFilepath.empty() && !mapinfo.luaData.DecompressToFile(mapinfo.luaFilepath, &mapinfo.luaChecksum))
        {
            if(ci)
                ci->CI_Error(CE_WRONGMAP);
            Stop();
            return true;
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
                    return true;
                }

                const libsiedler2::ArchivItem_Map_Header& header = checkedCast<const glArchivItem_Map*>(map.get(0))->getHeader();

                RTTR_Assert(!gameLobby);
                gameLobby.reset(new GameLobby(false, IsHost(), header.getNumPlayers()));
                mapinfo.title = cvStringToUTF8(header.getName());
            }
            break;
            case MAPTYPE_SAVEGAME:
            {
                mapinfo.savegame.reset(new Savegame);
                if(!mapinfo.savegame->Load(mapinfo.filepath, true, false))
                {
                    if(ci)
                        ci->CI_Error(CE_WRONGMAP);
                    Stop();
                    return true;
                }

                RTTR_Assert(!gameLobby);
                gameLobby.reset(new GameLobby(true, IsHost(), mapinfo.savegame->GetNumPlayers()));
                mapinfo.title = mapinfo.savegame->GetMapName();
            }
            break;
        }

        if(mainPlayer.playerId >= gameLobby->getNumPlayers())
        {
            if(ci)
                ci->CI_Error(CE_WRONGMAP);
            Stop();
            return true;
        }
        mainPlayer.sendMsgAsync(new GameMessage_Map_Checksum(mapinfo.mapChecksum, mapinfo.luaChecksum));

        LOG.writeToFile(">>>NMS_MAP_CHECKSUM(%u)\n") % mapinfo.mapChecksum;
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// map-checksum
/// @param message  Nachricht, welche ausgeführt wird
bool GameClient::OnGameMessage(const GameMessage_Map_ChecksumOK& msg)
{
    LOG.writeToFile("<<< NMS_MAP_CHECKSUM(%d)\n") % (msg.correct ? 1 : 0);

    if(!msg.correct)
    {
        if(ci)
            ci->CI_Error(CE_WRONGMAP);

        Stop();
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// server typ
/// @param message  Nachricht, welche ausgeführt wird
bool GameClient::OnGameMessage(const GameMessage_GGSChange& msg)
{
    RTTR_Assert(state == CS_CONFIG);
    LOG.writeToFile("<<< NMS_GGS_CHANGE\n");

    gameLobby->getSettings() = msg.ggs;

    if(ci)
        ci->CI_GGSChanged(msg.ggs);
    return true;
}

bool GameClient::OnGameMessage(const GameMessage_RemoveLua& msg)
{
    RTTR_Assert(state == CS_CONNECT || state == CS_CONFIG);
    mapinfo.luaFilepath.clear();
    mapinfo.luaData.Clear();
    mapinfo.luaChecksum = 0;
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// NFC Antwort vom Server
/// @param message  Nachricht, welche ausgeführt wird
bool GameClient::OnGameMessage(const GameMessage_GameCommand& msg)
{
    if(state != CS_LOADING && state != CS_GAME)
        return true;
    ClientPlayer* player = clientPlayers->get(msg.player);
    if(!player)
        return true;
    // LOG.writeToFile("CLIENT <<< GC %u\n") % unsigned(msg.player);
    // If this is our GC then we must have no other command as we need to execute this before we even send the next one
    RTTR_Assert(msg.player != mainPlayer.playerId || player->gcsToExecute.empty());
    // Nachricht in Queue einhängen
    player->gcsToExecute.push(msg.gcs);
    return true;
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

    mainPlayer.sendMsgAsync(new GameMessage_Server_Speed(framesinfo.gfLengthReq));
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

    mainPlayer.sendMsgAsync(new GameMessage_Server_Speed(framesinfo.gfLengthReq));
}

void GameClient::IncreaseReplaySpeed()
{
    if(replayMode)
    {
        if(framesinfo.gf_length > 10)
            framesinfo.gf_length -= 10;
        else if(framesinfo.gf_length == 10)
            framesinfo.gf_length = 1;
    }
}

void GameClient::DecreaseReplaySpeed()
{
    if(replayMode)
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
bool GameClient::OnGameMessage(const GameMessage_Server_NWFDone& msg)
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
    return true;
}

/**
 *  NFC Pause-Nachricht von Server
 *
 *  @param[in] message Nachricht, welche ausgeführt wird
 */
bool GameClient::OnGameMessage(const GameMessage_Pause& msg)
{
    if(framesinfo.isPaused == msg.paused)
        return true;
    framesinfo.isPaused = msg.paused;

    LOG.writeToFile("<<< NMS_NFC_PAUSE(%1%)\n") % msg.paused;

    if(msg.paused)
        ci->CI_GamePaused();
    else
        ci->CI_GameResumed();
    return true;
}

/**
 *  NFC GetAsyncLog von Server
 *
 *  @param[in] message Nachricht, welche ausgeführt wird
 */
bool GameClient::OnGameMessage(const GameMessage_GetAsyncLog& /*msg*/)
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
            mainPlayer.sendMsgAsync(new GameMessage_SendAsyncLog(part, false));
            part.clear();
        }
    }

    mainPlayer.sendMsgAsync(new GameMessage_SendAsyncLog(part, true));
    return true;
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
        if(replayMode)
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
                if(clientPlayers->checkForLaggingPlayers())
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

        RTTR_Assert(replayMode || curGF <= framesinfo.gfNrServer + framesinfo.nwf_length);
        // Store this timestamp
        framesinfo.lastTime = currentTime;
        // Reset frameTime
        framesinfo.frameTime = 0;

        HandleAutosave();

        // GF-Ende im Replay aktualisieren
        if(!replayMode)
            replayinfo->replay.UpdateLastGF(curGF);
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
    if(!SETTINGS.interface.autosave_interval || replayMode)
        return;

    // Alle .... GF
    if(GetGFNumber() % SETTINGS.interface.autosave_interval == 0)
    {
        std::string tmp = RTTRCONFIG.ExpandPath(FILE_PATHS[85]) + "/";

        if(mapinfo.title.empty())
        {
            tmp += _("Auto-Save");
            tmp += ".sav";
        } else
        {
            tmp += mapinfo.title + " (";
            tmp += _("Auto-Save");
            tmp += ").sav";
        }

        SaveToFile(tmp);
    }
}

/// Führt notwendige Dinge für nächsten GF aus
void GameClient::NextGF()
{
    game->RunGF();

    if(human_ai)
    {
        human_ai->RunGF(GetGFNumber(), (GetGFNumber() % framesinfo.nwf_length == 0));

        const std::vector<gc::GameCommandPtr>& ai_gcs = human_ai->GetGameCommands();
        gameCommands_.insert(gameCommands_.end(), ai_gcs.begin(), ai_gcs.end());
        human_ai->FetchGameCommands();
    }
}

void GameClient::ExecuteAllGCs(uint8_t playerId, const PlayerGameCommands& gcs)
{
    BOOST_FOREACH(const gc::GameCommandPtr& gc, gcs.gcs)
        gc->Execute(game->world, playerId);
}

void GameClient::SendNothingNC()
{
    mainPlayer.sendMsgAsync(new GameMessage_GameCommand(0xFF, AsyncChecksum::create(*game), std::vector<gc::GameCommandPtr>()));
}

void GameClient::WritePlayerInfo(SavedFile& file)
{
    RTTR_Assert(state == CS_LOADING || state == CS_GAME);
    // Spielerdaten
    for(unsigned i = 0; i < game->world.GetNumPlayers(); ++i)
        file.AddPlayer(game->world.GetPlayer(i));
}

void GameClient::StartReplayRecording(const unsigned random_init)
{
    replayinfo.reset(new ReplayInfo);
    replayinfo->fileName = s25util::Time::FormatTime("%Y-%m-%d_%H-%i-%s") + ".rpl";
    replayinfo->replay.random_init = random_init;

    WritePlayerInfo(replayinfo->replay);
    replayinfo->replay.ggs = game->ggs;

    // Datei speichern
    if(!replayinfo->replay.StartRecording(RTTRCONFIG.ExpandPath(FILE_PATHS[51]) + "/" + replayinfo->fileName, mapinfo))
    {
        LOG.write("GameClient::WriteReplayHeader: WARNING: File couldn't be opened. Don't use a replayinfo.\n");
        replayinfo.reset();
    }
}

bool GameClient::StartReplay(const std::string& path)
{
    RTTR_Assert(state == CS_STOPPED);
    mapinfo.Clear();
    replayinfo.reset(new ReplayInfo);

    if(!replayinfo->replay.LoadHeader(path, true) || !replayinfo->replay.LoadGameData(mapinfo)) //-V807
    {
        LOG.write(_("Invalid Replay %1%! Reason: %2%\n")) % path
          % (replayinfo->replay.GetLastErrorMsg().empty() ? _("Unknown") : replayinfo->replay.GetLastErrorMsg());
        if(ci)
            ci->CI_Error(CE_WRONGMAP);
        replayinfo.reset();
        return false;
    }
    replayinfo->fileName = bfs::path(replayinfo->replay.GetFile().getFilePath()).filename().string();

    gameLobby.reset(new GameLobby(true, true, replayinfo->replay.GetNumPlayers()));

    for(unsigned i = 0; i < replayinfo->replay.GetNumPlayers(); ++i)
        gameLobby->getPlayer(i) = JoinPlayerInfo(replayinfo->replay.GetPlayer(i));

    bool playerFound = false;
    // Find a player to spectate from
    // First find a human player
    for(unsigned char i = 0; i < gameLobby->getNumPlayers(); ++i)
    {
        if(gameLobby->getPlayer(i).ps == PS_OCCUPIED)
        {
            mainPlayer.playerId = i;
            playerFound = true;
            break;
        }
    }
    if(!playerFound)
    {
        // If no human found, take the first AI
        for(unsigned char i = 0; i < gameLobby->getNumPlayers(); ++i)
        {
            if(gameLobby->getPlayer(i).ps == PS_AI)
            {
                mainPlayer.playerId = i;
                break;
            }
        }
    }

    // GGS-Daten
    gameLobby->getSettings() = replayinfo->replay.ggs;

    switch(mapinfo.type)
    {
        default: break;
        case MAPTYPE_OLDMAP:
        {
            // Richtigen Pfad zur Map erstellen
            bfs::path mapFilePath = bfs::path(RTTRCONFIG.ExpandPath(FILE_PATHS[48])) / bfs::path(mapinfo.filepath).filename();
            mapinfo.filepath = mapFilePath.string();
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
                mapinfo.luaFilepath = mapFilePath.replace_extension("lua").string();
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

    replayMode = true;
    replayinfo->async = 0;
    replayinfo->end = false;

    try
    {
        StartGame(replayinfo->replay.random_init);
    } catch(SerializedGameData::Error& error)
    {
        LOG.write(_("Error when loading game from replay: %s\n")) % error.what();
        if(ci)
            ci->CI_Error(CE_WRONGMAP);
        Stop();
        return false;
    }

    replayinfo->replay.ReadGF(&replayinfo->next_gf);

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

unsigned GameClient::Interpolate(unsigned max_val, const GameEvent* ev)
{
    RTTR_Assert(ev);
    unsigned elapsedTime = (GetGFNumber() - ev->startGF) * framesinfo.gf_length + framesinfo.frameTime;
    unsigned duration = ev->length * framesinfo.gf_length;
    unsigned result = (max_val * elapsedTime) / duration;
    if(result >= max_val)
        RTTR_Assert(result < max_val);
    return result;
}

int GameClient::Interpolate(int x1, int x2, const GameEvent* ev)
{
    RTTR_Assert(ev);
    unsigned elapsedTime = (GetGFNumber() - ev->startGF) * framesinfo.gf_length + framesinfo.frameTime;
    unsigned duration = ev->length * framesinfo.gf_length;
    return x1 + ((x2 - x1) * int(elapsedTime)) / int(duration);
}

void GameClient::ServerLost()
{
    Stop();
    if(ci)
        ci->CI_Error(CE_CONNECTIONLOST);
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

    if(!replayMode)
    {
        // unpause before skipping
        GAMESERVER.SetPaused(false);
        GAMESERVER.skiptogf = gf;
        skiptogf = gf;
        LOG.write("jumping from gf %i to gf %i \n") % GetGFNumber() % gf;
        return;
    }

    SetPause(false);

    // GFs überspringen
    for(unsigned i = GetGFNumber(); i < gf; ++i)
    {
        if(i % 1000 == 0)
        {
            RoadBuildState road;
            road.mode = RM_DISABLED;

            // spiel aktualisieren
            gwv.Draw(road, MapPoint::Invalid(), false);

            // text oben noch hinschreiben
            boost::format nwfString(_("current GF: %u - still fast forwarding: %d GFs left (%d %%)"));
            nwfString % GetGFNumber() % (gf - i) % (i * 100 / gf);
            LargeFont->Draw(DrawPoint(VIDEODRIVER.GetScreenSize() / 2u), nwfString.str(), glArchivItem_Font::DF_CENTER, 0xFFFFFF00);

            VIDEODRIVER.SwapBuffers();
        }
        ExecuteGameFrame(true);
        // LOG.write(("jumping: now at gf %i\n", framesinfo.nr);
    }

    // Spiel pausieren & text ausgabe wie lang das jetzt gedauert hat
    unsigned ticks = VIDEODRIVER.GetTickCount() - start_ticks;
    boost::format text(_("Jump finished (%1$.3g seconds)."));
    text % (ticks / 1000.0);
    ci->CI_Chat(mainPlayer.playerId, CD_SYSTEM, text.str());
    SetPause(true);
}

void GameClient::SystemChat(const std::string& text, unsigned char player)
{
    if(player == 0xFF)
        player = mainPlayer.playerId;
    ci->CI_Chat(player, CD_SYSTEM, text);
}

bool GameClient::SaveToFile(const std::string& filename)
{
    GameMessage_System_Chat saveAnnouncement(0xFF, "Saving game...");
    mainPlayer.sendMsg(saveAnnouncement);

    // Mond malen
    Point<int> moonPos = VIDEODRIVER.GetMousePos();
    moonPos.y -= 40;
    LOADER.GetImageN("resource", 33)->DrawFull(moonPos);
    VIDEODRIVER.SwapBuffers();

    Savegame save;

    WritePlayerInfo(save);

    // GGS-Daten
    save.ggs = game->ggs;

    save.start_gf = GetGFNumber();

    // Enable/Disable debugging of savegames
    save.sgd.debugMode = SETTINGS.global.debugMode;

    // Spiel serialisieren
    save.sgd.MakeSnapshot(game->world);

    // Und alles speichern
    return save.Save(filename, mapinfo.title);
}

void GameClient::ResetVisualSettings()
{
    GetPlayer(mainPlayer.playerId).FillVisualSettings(visual_settings);
}

void GameClient::SetPause(bool pause)
{
    if(state == CS_STOPPED)
    {
        // Simply do it
        framesinfo.isPaused = pause;
        framesinfo.frameTime = 0;
    } else if(replayMode)
    {
        framesinfo.isPaused = pause;
        framesinfo.frameTime = 0;
    } else if(IsHost())
        GAMESERVER.SetPaused(pause);
}

void GameClient::ToggleReplayFOW()
{
    replayinfo->all_visible = !replayinfo->all_visible;
}

bool GameClient::IsReplayFOWDisabled() const
{
    return replayinfo->all_visible;
}

unsigned GameClient::GetLastReplayGF() const
{
    return replayinfo->replay.GetLastGF();
}

bool GameClient::AddGC(gc::GameCommand* gc)
{
    // Nicht in der Pause oder wenn er besiegt wurde
    if(framesinfo.isPaused || GetPlayer(mainPlayer.playerId).IsDefeated() || IsReplayModeOn())
    {
        delete gc;
        return false;
    }

    gameCommands_.push_back(gc);
    return true;
}

unsigned GameClient::GetNumPlayers() const
{
    RTTR_Assert(state == CS_LOADING || state == CS_GAME);
    return game->world.GetNumPlayers();
}

GamePlayer& GameClient::GetPlayer(const unsigned id)
{
    RTTR_Assert(state == CS_LOADING || state == CS_GAME);
    RTTR_Assert(id < GetNumPlayers());
    return game->world.GetPlayer(id);
}

AIPlayer* GameClient::CreateAIPlayer(unsigned playerId, const AI::Info& aiInfo)
{
    return AIFactory::Create(aiInfo, playerId, game->world);
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

const std::string& GameClient::GetReplayFileName() const
{
    static std::string emptyString;
    return replayinfo ? replayinfo->fileName : emptyString;
}

Replay& GameClient::GetReplay()
{
    RTTR_Assert(replayinfo);
    return replayinfo->replay;
}

boost::shared_ptr<const ClientPlayers> GameClient::GetPlayers() const
{
    return clientPlayers;
}

/// Is tournament mode activated (0 if not)? Returns the durations of the tournament mode in gf otherwise
unsigned GameClient::GetTournamentModeDuration() const
{
    if(unsigned(game->ggs.objective) >= NUM_OBJECTIVESS)
        return TOURNAMENT_MODES_DURATION[game->ggs.objective - NUM_OBJECTIVESS] * 60 * 1000 / framesinfo.gf_length;
    else
        return 0;
}

void GameClient::ToggleHumanAIPlayer()
{
    RTTR_Assert(!IsReplayModeOn());
    if(human_ai)
        human_ai.reset();
    else
        human_ai.reset(CreateAIPlayer(mainPlayer.playerId, AI::Info(AI::DEFAULT, AI::EASY)));
}

void GameClient::RequestSwapToPlayer(const unsigned char newId)
{
    if(state != CS_GAME)
        return;
    GamePlayer& player = GetPlayer(newId);
    if(player.ps == PS_AI && player.aiInfo.type == AI::DUMMY)
        mainPlayer.sendMsgAsync(new GameMessage_Player_Swap(0xFF, newId));
}
