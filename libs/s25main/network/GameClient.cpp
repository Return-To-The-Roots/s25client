// Copyright (c) 2005 - 2020 Settlers Freaks (sf-team at siedler25.org)
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

#include "GameClient.h"
#include "CreateServerInfo.h"
#include "EventManager.h"
#include "Game.h"
#include "GameEvent.h"
#include "GameLobby.h"
#include "GameManager.h"
#include "GameMessage_GameCommand.h"
#include "JoinPlayerInfo.h"
#include "Loader.h"
#include "NWFInfo.h"
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
#include "helpers/containerUtils.h"
#include "helpers/format.hpp"
#include "lua/LuaInterfaceBase.h"
#include "network/ClientInterface.h"
#include "network/GameMessages.h"
#include "network/GameServer.h"
#include "ogl/FontStyle.h"
#include "ogl/glArchivItem_Bitmap.h"
#include "ogl/glArchivItem_Map.h"
#include "ogl/glFont.h"
#include "random/Random.h"
#include "world/GameWorld.h"
#include "world/GameWorldView.h"
#include "gameTypes/RoadBuildState.h"
#include "gameData/GameConsts.h"
#include "libsiedler2/ArchivItem_Map_Header.h"
#include "libsiedler2/prototypen.h"
#include "s25util/SocketSet.h"
#include "s25util/StringConversion.h"
#include "s25util/System.h"
#include "s25util/fileFuncs.h"
#include "s25util/strFuncs.h"
#include "s25util/utf8.h"
#include <boost/filesystem.hpp>
#include <helpers/chronoIO.h>
#include <memory>

void GameClient::ClientConfig::Clear()
{
    server.clear();
    gameName.clear();
    password.clear();
    port = 0;
    isHost = false;
}

GameClient::GameClient() : skiptogf(0), mainPlayer(0), state(CS_STOPPED), ci(nullptr), replayMode(false) {}

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
        if(host)
            GAMESERVER.Stop();
        return false;
    }

    state = CS_CONNECT;

    if(ci)
        ci->CI_NextConnectState(CS_WAITFORANSWER);

    // Es wird kein Replay abgespielt, sondern dies ist ein richtiges Spiel
    replayMode = false;

    return true;
}

bool GameClient::HostGame(const CreateServerInfo& csi, const std::string& map_path, MapType map_type)
{
    std::string hostPw = createRandString(20);
    return GAMESERVER.Start(csi, map_path, map_type, hostPw) && Connect("localhost", hostPw, csi.type, csi.port, true, csi.ipv6);
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

    if(state == CS_LOADED)
    {
        // All players ready?
        if(nwfInfo->isReady())
            OnGameStart();
    } else if(state == CS_GAME)
        ExecuteGameFrame();

    // maximal 10 Pakete verschicken
    mainPlayer.sendMsgs(10);

    mainPlayer.executeMsgs(*this);
}

/**
 *  Stoppt das Spiel
 */
void GameClient::Stop()
{
    if(state == CS_STOPPED)
        return;

    if(game)
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
        replayinfo->replay.Close();
        replayinfo.reset();
    }

    mainPlayer.closeConnection();

    // clear jump target
    skiptogf = 0;

    // Consistency check: No game, no lobby remaining
    RTTR_Assert(!game);
    RTTR_Assert(!gameLobby);

    state = CS_STOPPED;
    LOG.write("client state changed to stop\n");
}

std::shared_ptr<GameLobby> GameClient::GetGameLobby()
{
    RTTR_Assert(state == CS_CONFIG);
    RTTR_Assert(gameLobby);
    return gameLobby;
}

const AIPlayer* GameClient::GetAIPlayer(unsigned id) const
{
    if(!game)
        return nullptr;
    return game->GetAIPlayer(id);
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
    Position moonPos = VIDEODRIVER.GetMousePos();
    moonPos.y -= 40;
    LOADER.GetImageN("resource", 33)->DrawFull(moonPos);
    VIDEODRIVER.SwapBuffers();

    // Start in pause mode
    framesinfo.isPaused = true;

    // Je nach Geschwindigkeit GF-Länge einstellen
    framesinfo.gf_length = FramesInfo::milliseconds32_t(SPEED_GF_LENGTHS[gameLobby->getSettings().speed]);
    framesinfo.gfLengthReq = framesinfo.gf_length;

    // Random-Generator initialisieren
    RANDOM.Init(random_init);

    if(!IsReplayModeOn() && mapinfo.savegame && !mapinfo.savegame->Load(mapinfo.filepath, true, true))
    {
        OnError(CE_INVALID_MAP);
        return;
    }

    // If we have a savegame, start at its first GF, else at 0
    unsigned startGF = (mapinfo.type == MAPTYPE_SAVEGAME) ? mapinfo.savegame->start_gf : 0;
    // Create the game
    game = std::make_shared<Game>(gameLobby->getSettings(), startGF,
                                  std::vector<PlayerInfo>(gameLobby->getPlayers().begin(), gameLobby->getPlayers().end()));
    if(!IsReplayModeOn())
    {
        for(unsigned id = 0; id < gameLobby->getNumPlayers(); id++)
        {
            if(gameLobby->getPlayer(id).isUsed())
                nwfInfo->addPlayer(id);
        }
    }
    // Release lobby
    gameLobby.reset();

    state = CS_LOADING;

    if(ci)
        ci->CI_GameLoading(game);

    // Get standard settings before they get overwritten
    GetPlayer(mainPlayer.playerId).FillVisualSettings(default_settings);

    GameWorld& gameWorld = game->world_;
    if(mapinfo.savegame)
        mapinfo.savegame->sgd.ReadSnapshot(game);
    else
    {
        RTTR_Assert(mapinfo.type != MAPTYPE_SAVEGAME);
        /// Startbündnisse setzen
        for(unsigned i = 0; i < gameWorld.GetNumPlayers(); ++i)
            gameWorld.GetPlayer(i).MakeStartPacts();

        if(!gameWorld.LoadMap(game, mapinfo.filepath, mapinfo.luaFilepath))
        {
            OnError(CE_INVALID_MAP);
            return;
        }

        /// Evtl. Goldvorkommen ändern
        Resource::Type target; // löschen
        switch(game->ggs_.getSelection(AddonId::CHANGE_GOLD_DEPOSITS))
        {
            case 0:
            default: target = Resource::Gold; break;
            case 1: target = Resource::Nothing; break;
            case 2: target = Resource::Iron; break;
            case 3: target = Resource::Coal; break;
            case 4: target = Resource::Granite; break;
        }
        gameWorld.ConvertMineResourceTypes(Resource::Gold, target);
        gameWorld.PlaceAndFixWater();
    }
    gameWorld.InitAfterLoad();

    // Update visual settings
    ResetVisualSettings();

    if(!replayMode)
    {
        RTTR_Assert(!replayinfo);
        StartReplayRecording(random_init);
    }

    // Daten nach dem Schreiben des Replays ggf wieder löschen
    mapinfo.mapData.Clear();
}

void GameClient::GameLoaded()
{
    RTTR_Assert(state == CS_LOADING);

    state = CS_LOADED;

    if(replayMode)
        OnGameStart();
    else
    {
        // Notify server that we are ready
        if(IsHost())
        {
            for(unsigned id = 0; id < GetNumPlayers(); id++)
            {
                if(GetPlayer(id).ps == PS_AI)
                {
                    game->AddAIPlayer(CreateAIPlayer(id, GetPlayer(id).aiInfo));
                    SendNothingNC(id);
                }
            }
        }
        SendNothingNC();
    }
}

void GameClient::ExitGame()
{
    RTTR_Assert(state == CS_GAME || state == CS_LOADED || state == CS_LOADING);
    game.reset();
    nwfInfo.reset();
    // Clear remaining commands
    gameCommands_.clear();
}

unsigned GameClient::GetGFNumber() const
{
    return game->em_->GetCurrentGF();
}

/**
 *  Ping-Nachricht.
 */
bool GameClient::OnGameMessage(const GameMessage_Ping& /*msg*/)
{
    mainPlayer.sendMsgAsync(new GameMessage_Pong());
    return true;
}

/**
 *  Player-ID-Nachricht.
 */
bool GameClient::OnGameMessage(const GameMessage_Player_Id& msg)
{
    if(state != CS_CONNECT)
        return true;
    // haben wir eine ungültige ID erhalten? (aka Server-Voll)
    if(msg.player == GameMessageWithPlayer::NO_PLAYER_ID)
    {
        OnError(CE_SERVER_FULL);
        return true;
    }

    mainPlayer.playerId = msg.player;

    // Server-Typ senden
    mainPlayer.sendMsgAsync(new GameMessage_Server_Type(clientconfig.servertyp, RTTR_Version::GetRevision()));
    return true;
}

/**
 *  Player-List-Nachricht.
 */
bool GameClient::OnGameMessage(const GameMessage_Player_List& msg)
{
    if(state != CS_CONNECT && state != CS_CONFIG)
        return true;
    RTTR_Assert(gameLobby);
    RTTR_Assert(gameLobby->getNumPlayers() == msg.playerInfos.size());
    if(gameLobby->getNumPlayers() != msg.playerInfos.size())
        return true;

    for(unsigned i = 0; i < gameLobby->getNumPlayers(); ++i)
        gameLobby->getPlayer(i) = msg.playerInfos[i];

    if(state != CS_CONFIG)
    {
        state = CS_CONFIG;
        if(ci)
            ci->CI_NextConnectState(CS_FINISHED);
    }
    return true;
}

bool GameClient::OnGameMessage(const GameMessage_Player_Name& msg)
{
    if(state != CS_CONFIG)
        return true;
    if(msg.player >= gameLobby->getNumPlayers())
        return true;
    gameLobby->getPlayer(msg.player).name = msg.playername;
    if(ci)
        ci->CI_PlayerDataChanged(msg.player);
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// player joined
/// @param message  Nachricht, welche ausgeführt wird
bool GameClient::OnGameMessage(const GameMessage_Player_New& msg)
{
    if(state != CS_CONFIG)
        return true;

    if(msg.player >= gameLobby->getNumPlayers())
        return true;

    JoinPlayerInfo& playerInfo = gameLobby->getPlayer(msg.player);

    playerInfo.name = msg.name;
    playerInfo.ps = PS_OCCUPIED;
    playerInfo.ping = 0;

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
    } else if(state == CS_LOADING || state == CS_LOADED || state == CS_GAME)
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
bool GameClient::OnGameMessage(const GameMessage_Player_State& msg)
{
    if(state != CS_CONFIG)
        return true;

    if(msg.player >= gameLobby->getNumPlayers())
        return true;

    JoinPlayerInfo& playerInfo = gameLobby->getPlayer(msg.player);
    bool wasUsed = playerInfo.isUsed();
    playerInfo.ps = msg.ps;
    playerInfo.aiInfo = msg.aiInfo;

    if(ci)
    {
        if(playerInfo.isUsed())
            ci->CI_NewPlayer(msg.player);
        else if(wasUsed)
            ci->CI_PlayerLeft(msg.player);
        else
            ci->CI_PlayerDataChanged(msg.player);
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// nation button gedrückt
/// @param message  Nachricht, welche ausgeführt wird
bool GameClient::OnGameMessage(const GameMessage_Player_Nation& msg)
{
    if(state != CS_CONFIG)
        return true;

    if(msg.player >= gameLobby->getNumPlayers())
        return true;

    gameLobby->getPlayer(msg.player).nation = msg.nation;

    if(ci)
        ci->CI_PlayerDataChanged(msg.player);
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// team button gedrückt
/// @param message  Nachricht, welche ausgeführt wird
bool GameClient::OnGameMessage(const GameMessage_Player_Team& msg)
{
    if(state != CS_CONFIG)
        return true;

    if(msg.player >= gameLobby->getNumPlayers())
        return true;

    gameLobby->getPlayer(msg.player).team = msg.team;

    if(ci)
        ci->CI_PlayerDataChanged(msg.player);
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// color button gedrückt
/// @param message  Nachricht, welche ausgeführt wird
bool GameClient::OnGameMessage(const GameMessage_Player_Color& msg)
{
    if(state != CS_CONFIG)
        return true;

    if(msg.player >= gameLobby->getNumPlayers())
        return true;

    gameLobby->getPlayer(msg.player).color = msg.color;

    if(ci)
        ci->CI_PlayerDataChanged(msg.player);
    return true;
}

/**
 *  Ready-state eines Spielers hat sich geändert.
 *
 *  @param[in] message Nachricht, welche ausgeführt wird
 */
bool GameClient::OnGameMessage(const GameMessage_Player_Ready& msg)
{
    if(state != CS_CONFIG)
        return true;

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
    if(state == CS_CONFIG)
    {
        if(msg.player >= gameLobby->getNumPlayers())
            return true;
        gameLobby->getPlayer(msg.player).ps = PS_FREE;
    } else if(state == CS_LOADING || state == CS_LOADED || state == CS_GAME)
    {
        // Im Spiel anzeigen, dass der Spieler das Spiel verlassen hat
        GamePlayer& player = GetPlayer(msg.player);
        if(player.ps != PS_AI)
        {
            player.ps = PS_AI;
            player.aiInfo = AI::Info(AI::DUMMY);
            // Host has to handle it
            if(IsHost())
            {
                game->AddAIPlayer(CreateAIPlayer(msg.player, player.aiInfo));
                SendNothingNC(msg.player);
            }
        }
    } else
        return true;

    if(ci)
        ci->CI_PlayerLeft(msg.player);
    return true;
}

bool GameClient::OnGameMessage(const GameMessage_Player_Swap& msg)
{
    LOG.writeToFile("<<< NMS_PLAYER_SWAP(%u, %u)\n") % unsigned(msg.player) % unsigned(msg.player2);

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
    } else if(state == CS_LOADING || state == CS_LOADED || state == CS_GAME)
        ChangePlayerIngame(msg.player, msg.player2);
    else
        return true;
    mainPlayer.sendMsgAsync(new GameMessage_Player_SwapConfirm(msg.player, msg.player2));
    return true;
}

/**
 *  Server-Typ-Nachricht.
 */
bool GameClient::OnGameMessage(const GameMessage_Server_TypeOK& msg)
{
    if(state != CS_CONNECT)
        return true;

    switch(msg.err_code)
    {
        case 0: // ok
            break;

        default:
        case 1:
        {
            OnError(CE_INVALID_SERVERTYPE);
            return true;
        }
        break;

        case 2:
        {
            OnError(CE_WRONG_VERSION);
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
    if(state != CS_CONNECT)
        return true;

    if(msg.password != "true")
    {
        OnError(CE_WRONG_PW);
        return true;
    }

    mainPlayer.sendMsgAsync(new GameMessage_Player_Name(0xFF, SETTINGS.lobby.name));
    mainPlayer.sendMsgAsync(new GameMessage_MapRequest(true));

    if(ci)
        ci->CI_NextConnectState(CS_QUERYMAPNAME);
    return true;
}

/**
 *  Server-Name-Nachricht.
 */
bool GameClient::OnGameMessage(const GameMessage_Server_Name& msg)
{
    if(state != CS_CONNECT)
        return true;
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
    if(state != CS_CONFIG)
        return true;

    nwfInfo = std::make_shared<NWFInfo>();
    nwfInfo->init(msg.firstNwf, msg.cmdDelay);
    try
    {
        StartGame(msg.random_init);
    } catch(SerializedGameData::Error& error)
    {
        LOG.write("Error when loading game: %s\n") % error.what();
        Stop();
        GAMEMANAGER.ShowMenu();
    }
    return true;
}

/**
 *  Server-Chat-Nachricht.
 */
bool GameClient::OnGameMessage(const GameMessage_Chat& msg)
{
    if(msg.destination == CD_SYSTEM)
    {
        SystemChat(msg.text, msg.player);
        return true;
    }
    if(state == CS_GAME)
    {
        // Ingame message: Do some checking and logging
        if(msg.player >= game->world_.GetNumPlayers())
            return true;

        /// Mit im Replay aufzeichnen
        if(replayinfo && replayinfo->replay.IsRecording())
            replayinfo->replay.AddChatCommand(GetGFNumber(), msg.player, msg.destination, msg.text);

        GamePlayer& player = game->world_.GetPlayer(msg.player);

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
        checksum_list << GetPlayer(i).name << ": " << msg.checksums[i];
        if(i + 1 < msg.checksums.size())
            checksum_list << ", ";
    }

    // Fehler ausgeben (Konsole)!
    LOG.write(_("The Game is not in sync. Checksums of some players don't match."));
    LOG.write("\n%1%\n") % checksum_list.str();

    // Messenger im Game
    if(ci)
        ci->CI_Async(checksum_list.str());

    std::string fileName = s25util::Time::FormatTime("async_%Y-%m-%d_%H-%i-%s");
    fileName += "_" + s25util::toStringClassic(mainPlayer.playerId) + "_";
    fileName += GetPlayer(mainPlayer.playerId).name;

    std::string filePathSave = RTTRCONFIG.ExpandPath(s25::folders::save) + "/" + makePortableFileName(fileName + ".sav");
    std::string filePathLog = RTTRCONFIG.ExpandPath(s25::folders::logs) + "/" + makePortableFileName(fileName + "Player.log");
    RANDOM.SaveLog(filePathLog);
    SaveToFile(filePathSave);
    LOG.write(_("Async log saved at \"%s\",\ngame saved at \"%s\"\n")) % filePathLog % filePathSave;
    return true;
}

/**
 *  Server-Countdown-Nachricht.
 */
bool GameClient::OnGameMessage(const GameMessage_Countdown& msg)
{
    if(state != CS_CONFIG)
        return true;
    if(ci)
        ci->CI_Countdown(msg.countdown);
    return true;
}

/**
 *  Server-Cancel-Countdown-Nachricht.
 */
bool GameClient::OnGameMessage(const GameMessage_CancelCountdown& msg)
{
    if(state != CS_CONFIG)
        return true;
    if(ci)
        ci->CI_CancelCountdown(msg.error);
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
    if(state != CS_CONNECT)
        return true;

    // full path
    std::string portFilename = makePortableFileName(msg.filename);
    if(portFilename.empty())
    {
        LOG.write("Invalid filename received!\n");
        OnError(CE_INVALID_MAP);
    }
    mapinfo.filepath = RTTRCONFIG.ExpandPath(s25::folders::mapsPlayed) + "/" + portFilename;
    mapinfo.type = msg.mt;

    // lua script file path
    if(msg.luaLen > 0)
        mapinfo.luaFilepath = bfs::path(mapinfo.filepath).replace_extension("lua").string();
    else
        mapinfo.luaFilepath.clear();

    if(bfs::exists(mapinfo.filepath) && (mapinfo.luaFilepath.empty() || bfs::exists(mapinfo.luaFilepath)) && CreateLobby())
    {
        mapinfo.mapData.CompressFromFile(mapinfo.filepath, &mapinfo.mapChecksum);
        if(mapinfo.mapData.data.size() == msg.mapCompressedLen && mapinfo.mapData.length == msg.mapLen)
        {
            bool ok = true;
            if(!mapinfo.luaFilepath.empty())
            {
                mapinfo.luaData.CompressFromFile(mapinfo.luaFilepath, &mapinfo.luaChecksum);
                ok = (mapinfo.luaData.data.size() == msg.luaCompressedLen && mapinfo.luaData.length == msg.luaLen);
            }

            if(ok)
            {
                mainPlayer.sendMsgAsync(new GameMessage_Map_Checksum(mapinfo.mapChecksum, mapinfo.luaChecksum));
                return true;
            }
        }
        gameLobby.reset();
    }
    mapinfo.mapData.length = msg.mapLen;
    mapinfo.luaData.length = msg.luaLen;
    mapinfo.mapData.data.resize(msg.mapCompressedLen);
    mapinfo.luaData.data.resize(msg.luaCompressedLen);
    mainPlayer.sendMsgAsync(new GameMessage_MapRequest(false));
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// Kartendaten
/// @param message  Nachricht, welche ausgeführt wird
bool GameClient::OnGameMessage(const GameMessage_Map_Data& msg)
{
    if(state != CS_CONNECT)
        return true;

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
            OnError(CE_MAP_TRANSMISSION);
            return true;
        }
        if(!mapinfo.luaFilepath.empty() && !mapinfo.luaData.DecompressToFile(mapinfo.luaFilepath, &mapinfo.luaChecksum))
        {
            OnError(CE_MAP_TRANSMISSION);
            return true;
        }
        RTTR_Assert(!mapinfo.luaFilepath.empty() || mapinfo.luaChecksum == 0);

        if(!CreateLobby())
        {
            OnError(CE_MAP_TRANSMISSION);
            return true;
        }

        mainPlayer.sendMsgAsync(new GameMessage_Map_Checksum(mapinfo.mapChecksum, mapinfo.luaChecksum));
    }
    return true;
}

bool GameClient::OnGameMessage(const GameMessage_SkipToGF& msg)
{
    skiptogf = msg.targetGF;
    LOG.write("Jumping from GF %1% to GF %2%\n") % GetGFNumber() % skiptogf;
    return true;
}

void GameClient::OnError(ClientError error)
{
    if(ci)
        ci->CI_Error(error);
    Stop();
}

bool GameClient::CreateLobby()
{
    RTTR_Assert(!gameLobby);

    unsigned numPlayers;

    switch(mapinfo.type)
    {
        case MAPTYPE_OLDMAP:
        {
            libsiedler2::Archiv map;

            // Karteninformationen laden
            if(libsiedler2::loader::LoadMAP(mapinfo.filepath, map, true) != 0)
            {
                LOG.write("GameClient::OnMapData: ERROR: Map \"%s\", couldn't load header!\n") % mapinfo.filepath;
                return false;
            }

            const libsiedler2::ArchivItem_Map_Header& header = checkedCast<const glArchivItem_Map*>(map.get(0))->getHeader();
            numPlayers = header.getNumPlayers();
            mapinfo.title = s25util::ansiToUTF8(header.getName());
        }
        break;
        case MAPTYPE_SAVEGAME:
            mapinfo.savegame = std::make_unique<Savegame>();
            if(!mapinfo.savegame->Load(mapinfo.filepath, true, false))
                return false;

            numPlayers = mapinfo.savegame->GetNumPlayers();
            mapinfo.title = mapinfo.savegame->GetMapName();
            break;
        default: return false;
    }

    if(mainPlayer.playerId >= numPlayers)
        return false;

    gameLobby = std::make_shared<GameLobby>(mapinfo.type == MAPTYPE_SAVEGAME, IsHost(), numPlayers);
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// map-checksum
/// @param message  Nachricht, welche ausgeführt wird
bool GameClient::OnGameMessage(const GameMessage_Map_ChecksumOK& msg)
{
    if(state != CS_CONNECT)
        return true;
    LOG.writeToFile("<<< NMS_MAP_CHECKSUM(%d)\n") % (msg.correct ? 1 : 0);

    if(!msg.correct)
    {
        gameLobby.reset();
        if(msg.retryAllowed)
            mainPlayer.sendMsgAsync(new GameMessage_MapRequest(false));
        else
            OnError(CE_MAP_TRANSMISSION);
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// server typ
/// @param message  Nachricht, welche ausgeführt wird
bool GameClient::OnGameMessage(const GameMessage_GGSChange& msg)
{
    if(state != CS_CONFIG)
        return true;
    LOG.writeToFile("<<< NMS_GGS_CHANGE\n");

    gameLobby->getSettings() = msg.ggs;

    if(ci)
        ci->CI_GGSChanged(msg.ggs);
    return true;
}

bool GameClient::OnGameMessage(const GameMessage_RemoveLua&)
{
    if(state != CS_CONNECT && state != CS_CONFIG)
        return true;
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
    if(nwfInfo)
    {
        if(!nwfInfo->addPlayerCmds(msg.player, msg.cmds))
        {
            LOG.write("Could not add gamecommands for player %1%. He might be cheating!\n") % unsigned(msg.player);
            RTTR_Assert(false);
        }
    }
    return true;
}

void GameClient::IncreaseSpeed()
{
    const bool debugMode =
#ifndef NDEBUG
      true;
#else
      false;
#endif
    if(framesinfo.gfLengthReq > FramesInfo::milliseconds32_t(10))
        framesinfo.gfLengthReq -= FramesInfo::milliseconds32_t(10);
    else if((replayMode || debugMode) && framesinfo.gfLengthReq == FramesInfo::milliseconds32_t(10))
        framesinfo.gfLengthReq = FramesInfo::milliseconds32_t(1);
    else
        framesinfo.gfLengthReq = FramesInfo::milliseconds32_t(70);

    if(replayMode)
        framesinfo.gf_length = framesinfo.gfLengthReq;
    else
        mainPlayer.sendMsgAsync(new GameMessage_Speed(framesinfo.gfLengthReq.count()));
}

void GameClient::DecreaseSpeed()
{
    const bool debugMode =
#ifndef NDEBUG
      true;
#else
      false;
#endif

    FramesInfo::milliseconds32_t maxSpeed(replayMode ? 1000 : 70);

    if(framesinfo.gfLengthReq == maxSpeed)
        framesinfo.gfLengthReq = FramesInfo::milliseconds32_t(replayMode || debugMode ? 1 : 10);
    else if(framesinfo.gfLengthReq == FramesInfo::milliseconds32_t(1))
        framesinfo.gfLengthReq = FramesInfo::milliseconds32_t(10);
    else
        framesinfo.gfLengthReq += FramesInfo::milliseconds32_t(10);

    if(replayMode)
        framesinfo.gf_length = framesinfo.gfLengthReq;
    else
        mainPlayer.sendMsgAsync(new GameMessage_Speed(framesinfo.gfLengthReq.count()));
}

///////////////////////////////////////////////////////////////////////////////
/// NFC Done vom Server
/// @param message  Nachricht, welche ausgeführt wird
bool GameClient::OnGameMessage(const GameMessage_Server_NWFDone& msg)
{
    if(!nwfInfo)
        return true;

    if(!nwfInfo->addServerInfo(NWFServerInfo(msg.gf, msg.gf_length, msg.nextNWF)))
    {
        RTTR_Assert(false);
        LOG.write("Failed to add server info. Invalid server?\n");
    }

    return true;
}

/**
 *  Pause-Nachricht von Server
 *
 *  @param[in] message Nachricht, welche ausgeführt wird
 */
bool GameClient::OnGameMessage(const GameMessage_Pause& msg)
{
    if(state != CS_GAME)
        return true;
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
    if(state != CS_GAME)
        return true;
    std::string systemInfo = System::getCompilerName() + " @ " + System::getOSName();
    mainPlayer.sendMsgAsync(new GameMessage_AsyncLog(systemInfo));

    // AsyncLog an den Server senden

    std::vector<RandomEntry> async_log = RANDOM.GetAsyncLog();

    // stückeln...
    std::vector<RandomEntry> part;
    for(auto& it : async_log)
    {
        part.push_back(it);

        if(part.size() == 10)
        {
            mainPlayer.sendMsgAsync(new GameMessage_AsyncLog(part, false));
            part.clear();
        }
    }

    mainPlayer.sendMsgAsync(new GameMessage_AsyncLog(part, true));
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// testet ob ein Netwerkframe abgelaufen ist und führt dann ggf die Befehle aus
void GameClient::ExecuteGameFrame()
{
    if(framesinfo.isPaused)
        return; // Pause

    FramesInfo::UsedClock::time_point currentTime = FramesInfo::UsedClock::now();

    if(framesinfo.forcePauseLen.count())
    {
        if(currentTime - framesinfo.forcePauseStart > framesinfo.forcePauseLen)
            framesinfo.forcePauseLen = FramesInfo::milliseconds32_t::zero();
        else
            return; // Pause
    }

    const unsigned curGF = GetGFNumber();
    // Is it time for the next GF? If we are skipping, it is always time for the next GF
    if(skiptogf > curGF || (currentTime - framesinfo.lastTime) >= framesinfo.gf_length)
    {
        try
        {
            if(replayMode)
            {
                // In replay mode we have all commands in the file -> Execute them
                ExecuteGameFrame_Replay();
            } else
            {
                RTTR_Assert(curGF <= nwfInfo->getNextNWF());
                bool isNWF = (curGF == nwfInfo->getNextNWF());
                // Is it time for a NWF, handle that first
                if(isNWF)
                {
                    // If a player is lagging (we did not got his commands) "pause" the game by skipping the rest of this function
                    // -> Don't execute GF, don't autosave etc.
                    if(!nwfInfo->isReady())
                    {
                        // If a player is a few GFs behind, he will never catch up and always lag
                        // Hence, pause up to 4 GFs randomly before trying again to execute this NWF
                        // Do not reset frameTime or lastTime as this will mess up interpolation for drawing
                        framesinfo.forcePauseStart = currentTime;
                        framesinfo.forcePauseLen = (rand() * 4 * framesinfo.gf_length) / RAND_MAX;
                        return;
                    }

                    RTTR_Assert(nwfInfo->getServerInfo().gf == curGF);

                    ExecuteNWF();

                    FramesInfo::milliseconds32_t oldGFLen = framesinfo.gf_length;
                    nwfInfo->execute(framesinfo);
                    if(oldGFLen != framesinfo.gf_length)
                    {
                        LOG.write("Client: Speed changed at %1% from %2% to %3% (NWF: %4%)\n") % curGF % oldGFLen % framesinfo.gf_length
                          % framesinfo.nwf_length;
                    }
                }

                NextGF(isNWF);
                RTTR_Assert(curGF <= nwfInfo->getNextNWF());
                HandleAutosave();

                // GF-Ende im Replay aktualisieren
                if(replayinfo && replayinfo->replay.IsRecording())
                    replayinfo->replay.UpdateLastGF(curGF);
            }

            // Store this timestamp
            framesinfo.lastTime = currentTime;
            // Reset frameTime
            framesinfo.frameTime = FramesInfo::milliseconds32_t::zero();
        } catch(LuaExecutionError& e)
        {
            if(ci)
            {
                SystemChat((boost::format(_("Error during execution of lua script: %1\nGame stopped!")) % e.what()).str());
                ci->CI_Error(CE_INVALID_MAP);
            }
            Stop();
        }
        if(skiptogf == GetGFNumber())
            skiptogf = 0;
    } else
    {
        // Next GF not yet reached, just update the time in the current one for drawing
        framesinfo.frameTime = std::chrono::duration_cast<FramesInfo::milliseconds32_t>(currentTime - framesinfo.lastTime);
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
        std::string tmp = RTTRCONFIG.ExpandPath(s25::folders::save) + "/";

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
void GameClient::NextGF(bool wasNWF)
{
    for(AIPlayer& ai : game->aiPlayers_)
        ai.RunGF(GetGFNumber(), wasNWF);
    game->RunGF();
}

void GameClient::ExecuteAllGCs(uint8_t playerId, const PlayerGameCommands& gcs)
{
    for(const gc::GameCommandPtr& gc : gcs.gcs)
        gc->Execute(game->world_, playerId);
}

void GameClient::SendNothingNC(uint8_t player)
{
    mainPlayer.sendMsgAsync(new GameMessage_GameCommand(player, AsyncChecksum::create(*game), std::vector<gc::GameCommandPtr>()));
}

void GameClient::WritePlayerInfo(SavedFile& file)
{
    RTTR_Assert(state == CS_LOADING || state == CS_LOADED || state == CS_GAME);
    // Spielerdaten
    for(unsigned i = 0; i < GetNumPlayers(); ++i)
        file.AddPlayer(GetPlayer(i));
}

void GameClient::OnGameStart()
{
    if(state == CS_LOADED)
    {
        GAMEMANAGER.ResetAverageGFPS();
        framesinfo.lastTime = FramesInfo::UsedClock::now();
        state = CS_GAME;
        if(ci)
            ci->CI_GameStarted(game);
    } else if(state == CS_GAME && !game->IsStarted())
    {
        framesinfo.isPaused = replayMode;
        game->Start(!!mapinfo.savegame);
    }
}

void GameClient::SetTestPlayerId(unsigned id)
{
    mainPlayer.playerId = id;
}

void GameClient::StartReplayRecording(const unsigned random_init)
{
    replayinfo = std::make_unique<ReplayInfo>();
    replayinfo->fileName = s25util::Time::FormatTime("%Y-%m-%d_%H-%i-%s") + ".rpl";
    replayinfo->replay.random_init = random_init;

    WritePlayerInfo(replayinfo->replay);
    replayinfo->replay.ggs = game->ggs_;

    // Datei speichern
    if(!replayinfo->replay.StartRecording(RTTRCONFIG.ExpandPath(s25::folders::replays) + "/" + replayinfo->fileName, mapinfo))
    {
        LOG.write(_("Replayfile couldn't be opened. No replay will be recorded\n"));
        replayinfo.reset();
    }
}

bool GameClient::StartReplay(const std::string& path)
{
    RTTR_Assert(state == CS_STOPPED);
    mapinfo.Clear();
    replayinfo = std::make_unique<ReplayInfo>();

    if(!replayinfo->replay.LoadHeader(path, true) || !replayinfo->replay.LoadGameData(mapinfo)) //-V807
    {
        LOG.write(_("Invalid Replay %1%! Reason: %2%\n")) % path
          % (replayinfo->replay.GetLastErrorMsg().empty() ? _("Unknown") : replayinfo->replay.GetLastErrorMsg());
        OnError(CE_INVALID_MAP);
        replayinfo.reset();
        return false;
    }
    replayinfo->fileName = bfs::path(replayinfo->replay.GetFile().getFilePath()).filename().string();

    gameLobby = std::make_shared<GameLobby>(true, true, replayinfo->replay.GetNumPlayers());

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
            bfs::path mapFilePath = bfs::path(RTTRCONFIG.ExpandPath(s25::folders::mapsPlayed)) / bfs::path(mapinfo.filepath).filename();
            mapinfo.filepath = mapFilePath.string();
            if(!mapinfo.mapData.DecompressToFile(mapinfo.filepath))
            {
                LOG.write(_("Error decompressing map file"));
                OnError(CE_MAP_TRANSMISSION);
                return false;
            }
            if(mapinfo.luaData.length)
            {
                mapinfo.luaFilepath = mapFilePath.replace_extension("lua").string();
                if(!mapinfo.luaData.DecompressToFile(mapinfo.luaFilepath))
                {
                    LOG.write(_("Error decompressing lua file"));
                    OnError(CE_MAP_TRANSMISSION);
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
        OnError(CE_INVALID_MAP);
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
    using namespace std::chrono;
    const unsigned currenttime =
      std::chrono::duration_cast<FramesInfo::milliseconds32_t>((framesinfo.lastTime + framesinfo.frameTime).time_since_epoch()).count();
    return ((currenttime % unit) * max / unit + offset) % max;
}

unsigned GameClient::Interpolate(unsigned max_val, const GameEvent* ev)
{
    RTTR_Assert(ev);
    // TODO: Move to some animation system that is part of game
    FramesInfo::milliseconds32_t elapsedTime;
    if(state == CS_GAME)
        elapsedTime = (GetGFNumber() - ev->startGF) * framesinfo.gf_length + framesinfo.frameTime;
    else
        elapsedTime = FramesInfo::milliseconds32_t::zero();
    FramesInfo::milliseconds32_t duration = ev->length * framesinfo.gf_length;
    unsigned result = (max_val * elapsedTime) / duration;
    if(result >= max_val)
        RTTR_Assert(result < max_val); //-V547
    return result;
}

int GameClient::Interpolate(int x1, int x2, const GameEvent* ev)
{
    RTTR_Assert(ev);
    using milliseconds32_t = std::chrono::duration<int32_t, std::milli>;
    milliseconds32_t elapsedTime;
    if(state == CS_GAME)
        elapsedTime = (GetGFNumber() - ev->startGF) * framesinfo.gf_length + framesinfo.frameTime;
    else
        elapsedTime = milliseconds32_t::zero();
    milliseconds32_t duration = ev->length * framesinfo.gf_length;
    return x1 + ((x2 - x1) * elapsedTime) / duration;
}

void GameClient::ServerLost()
{
    OnError(CE_CONNECTION_LOST);
    // Stop game
    framesinfo.isPaused = true;
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
        SetPause(false);
        mainPlayer.sendMsgAsync(new GameMessage_SkipToGF(gf));
        return;
    }

    SetPause(false);
    skiptogf = gf;

    // GFs überspringen
    for(unsigned i = GetGFNumber(); i < skiptogf; ++i)
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
            LargeFont->Draw(DrawPoint(VIDEODRIVER.GetRenderSize() / 2u), nwfString.str(), FontStyle::CENTER, COLOR_YELLOW);

            VIDEODRIVER.SwapBuffers();
        }
        ExecuteGameFrame();
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
    if(!ci)
        return;
    if(player == 0xFF)
        player = mainPlayer.playerId;
    ci->CI_Chat(player, CD_SYSTEM, text);
}

bool GameClient::SaveToFile(const std::string& filename)
{
    mainPlayer.sendMsg(GameMessage_Chat(0xFF, CD_SYSTEM, "Saving game..."));

    // Mond malen
    Position moonPos = VIDEODRIVER.GetMousePos();
    moonPos.y -= 40;
    LOADER.GetImageN("resource", 33)->DrawFull(moonPos);
    VIDEODRIVER.SwapBuffers();

    Savegame save;

    WritePlayerInfo(save);

    // GGS-Daten
    save.ggs = game->ggs_;

    save.start_gf = GetGFNumber();

    // Enable/Disable debugging of savegames
    save.sgd.debugMode = SETTINGS.global.debugMode;

    try
    {
        // Spiel serialisieren
        save.sgd.MakeSnapshot(game);
        // Und alles speichern
        return save.Save(filename, mapinfo.title);
    } catch(std::exception& e)
    {
        OnGameMessage(GameMessage_Chat(0xFF, CD_SYSTEM, std::string("Error during saving: ") + e.what()));
        return false;
    }
}

void GameClient::ResetVisualSettings()
{
    GetPlayer(mainPlayer.playerId).FillVisualSettings(visual_settings);
}

void GameClient::SetPause(bool pause)
{
    if(state == CS_STOPPED)
    {
        // We can never continue from pause if stopped as the reason for stopping might be that the game was finished
        // However we allow to pause even when stopped so we can pause after we received the stop notification
        if(!pause)
            return;
        framesinfo.isPaused = true;
        framesinfo.frameTime = FramesInfo::milliseconds32_t::zero();
    } else if(replayMode)
    {
        framesinfo.isPaused = pause;
        framesinfo.frameTime = FramesInfo::milliseconds32_t::zero();
    } else if(IsHost())
    {
        // Pause instantly
        if(pause)
            framesinfo.isPaused = true;
        mainPlayer.sendMsgAsync(new GameMessage_Pause(pause));
    }
}

void GameClient::ToggleReplayFOW()
{
    if(replayinfo)
        replayinfo->all_visible = !replayinfo->all_visible;
}

bool GameClient::IsReplayFOWDisabled() const
{
    return replayMode && replayinfo->all_visible;
}

unsigned GameClient::GetLastReplayGF() const
{
    return replayinfo ? replayinfo->replay.GetLastGF() : 0u;
}

bool GameClient::AddGC(gc::GameCommandPtr gc)
{
    // Nicht in der Pause oder wenn er besiegt wurde
    if(framesinfo.isPaused || GetPlayer(mainPlayer.playerId).IsDefeated() || IsReplayModeOn())
        return false;

    gameCommands_.push_back(gc);
    return true;
}

unsigned GameClient::GetNumPlayers() const
{
    RTTR_Assert(state == CS_LOADING || state == CS_LOADED || state == CS_GAME);
    return game->world_.GetNumPlayers();
}

GamePlayer& GameClient::GetPlayer(const unsigned id)
{
    RTTR_Assert(state == CS_LOADING || state == CS_LOADED || state == CS_GAME);
    RTTR_Assert(id < GetNumPlayers());
    return game->world_.GetPlayer(id);
}

std::unique_ptr<AIPlayer> GameClient::CreateAIPlayer(unsigned playerId, const AI::Info& aiInfo)
{
    return AIFactory::Create(aiInfo, playerId, game->world_);
}

/// Wandelt eine GF-Angabe in eine Zeitangabe um (HH:MM:SS oder MM:SS wenn Stunden = 0)
std::string GameClient::FormatGFTime(const unsigned gf) const
{
    using seconds = std::chrono::duration<uint32_t, std::chrono::seconds::period>;
    using hours = std::chrono::duration<uint32_t, std::chrono::hours::period>;
    using minutes = std::chrono::duration<uint32_t, std::chrono::minutes::period>;
    using std::chrono::duration_cast;

    // In Sekunden umrechnen
    seconds numSeconds = duration_cast<seconds>(gf * framesinfo.gf_length);

    // Angaben rausfiltern
    hours numHours = duration_cast<hours>(numSeconds);
    numSeconds -= numHours;
    minutes numMinutes = duration_cast<hours>(numSeconds);
    numSeconds -= numMinutes;

    // ganze Stunden mit dabei? Dann entsprechend anderes format, ansonsten ignorieren wir die einfach
    if(numHours.count())
        return helpers::format("%02u:%02u:%02u", numHours.count(), numMinutes.count(), numSeconds.count());
    else
        return helpers::format("%02u:%02u", numMinutes.count(), numSeconds.count());
}

const std::string& GameClient::GetReplayFileName() const
{
    static std::string emptyString;
    return replayinfo ? replayinfo->fileName : emptyString;
}

Replay* GameClient::GetReplay()
{
    return replayinfo ? &replayinfo->replay : nullptr;
}

std::shared_ptr<const NWFInfo> GameClient::GetNWFInfo() const
{
    return nwfInfo;
}

/// Is tournament mode activated (0 if not)? Returns the durations of the tournament mode in gf otherwise
unsigned GameClient::GetTournamentModeDuration() const
{
    using namespace std::chrono;
    if(game && unsigned(game->ggs_.objective) >= NUM_OBJECTIVES)
        return minutes(TOURNAMENT_MODES_DURATION[game->ggs_.objective - NUM_OBJECTIVES]) / framesinfo.gf_length;
    else
        return 0;
}

void GameClient::ToggleHumanAIPlayer()
{
    RTTR_Assert(!IsReplayModeOn());
    auto it = helpers::find_if(game->aiPlayers_, [id = this->GetPlayerId()](const auto& player) { return player.GetPlayerId() == id; });
    if(it != game->aiPlayers_.end())
        game->aiPlayers_.erase(it);
    else
        game->AddAIPlayer(CreateAIPlayer(mainPlayer.playerId, AI::Info(AI::DEFAULT, AI::EASY)));
}

void GameClient::RequestSwapToPlayer(const unsigned char newId)
{
    if(state != CS_GAME)
        return;
    GamePlayer& player = GetPlayer(newId);
    if(player.ps == PS_AI && player.aiInfo.type == AI::DUMMY)
        mainPlayer.sendMsgAsync(new GameMessage_Player_Swap(0xFF, newId));
}
