// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

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
#include "addons/AddonEconomyModeGameLength.h"
#include "addons/const_addons.h"
#include "ai/AIPlayer.h"
#include "drivers/VideoDriverWrapper.h"
#include "factories/AIFactory.h"
#include "files.h"
#include "helpers/containerUtils.h"
#include "helpers/format.hpp"
#include "helpers/mathFuncs.h"
#include "lua/LuaInterfaceBase.h"
#include "network/ClientInterface.h"
#include "network/GameMessages.h"
#include "network/GameServer.h"
#include "ogl/FontStyle.h"
#include "ogl/glArchivItem_Bitmap.h"
#include "ogl/glFont.h"
#include "random/Random.h"
#include "random/randomIO.h"
#include "world/GameWorld.h"
#include "world/GameWorldView.h"
#include "world/MapLoader.h"
#include "gameTypes/RoadBuildState.h"
#include "gameData/GameConsts.h"
#include "libsiedler2/ArchivItem_Map.h"
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

GameClient::GameClient() : skiptogf(0), mainPlayer(0), state(ClientState::Stopped), ci(nullptr), replayMode(false) {}

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
bool GameClient::Connect(const std::string& server, const std::string& password, ServerType servertyp,
                         unsigned short port, bool host, bool use_ipv6)
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

    state = ClientState::Connect;

    if(ci)
        ci->CI_NextConnectState(ConnectState::WaitForAnswer);

    // Es wird kein Replay abgespielt, sondern dies ist ein richtiges Spiel
    replayMode = false;

    return true;
}

bool GameClient::HostGame(const CreateServerInfo& csi, const boost::filesystem::path& map_path, MapType map_type)
{
    std::string hostPw = createRandString(20);
    return GAMESERVER.Start(csi, map_path, map_type, hostPw)
           && Connect("localhost", hostPw, csi.type, csi.port, true, csi.ipv6);
}

/**
 *  Hauptschleife des Clients
 */
void GameClient::Run()
{
    if(state == ClientState::Stopped)
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

    if(state == ClientState::Loaded)
    {
        // All players ready?
        if(nwfInfo->isReady())
            OnGameStart();
    } else if(state == ClientState::Game)
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
    if(state == ClientState::Stopped)
        return;

    if(game)
        ExitGame();
    else if(state == ClientState::Connect || state == ClientState::Config)
        gameLobby.reset();

    if(IsHost())
        GAMESERVER.Stop();

    framesinfo.Clear();
    clientconfig.Clear();
    mapinfo.Clear();

    if(replayinfo)
    {
        if(replayinfo->replay.IsRecording())
            replayinfo->replay.StopRecording();
        replayinfo->replay.Close();
        replayinfo.reset();
    }

    mainPlayer.closeConnection();

    // clear jump target
    skiptogf = 0;

    // Consistency check: No game, no lobby remaining
    RTTR_Assert(!game);
    RTTR_Assert(!gameLobby);

    state = ClientState::Stopped;
    LOG.write("client state changed to stop\n");
}

std::shared_ptr<GameLobby> GameClient::GetGameLobby()
{
    RTTR_Assert(state == ClientState::Config);
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
    RTTR_Assert(state == ClientState::Config || (state == ClientState::Stopped && replayMode));

    // Mond malen
    Position moonPos = VIDEODRIVER.GetMousePos();
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

    if(!IsReplayModeOn() && mapinfo.savegame && !mapinfo.savegame->Load(mapinfo.filepath, SaveGameDataToLoad::All))
    {
        OnError(ClientError::InvalidMap);
        return;
    }

    // If we have a savegame, start at its first GF, else at 0
    unsigned startGF = (mapinfo.type == MapType::Savegame) ? mapinfo.savegame->start_gf : 0;
    // Create the game
    game =
      std::make_shared<Game>(std::move(gameLobby->getSettings()), startGF,
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

    state = ClientState::Loading;

    if(ci)
        ci->CI_GameLoading(game);

    // Get standard settings before they get overwritten
    GetPlayer(GetPlayerId()).FillVisualSettings(default_settings);

    GameWorld& gameWorld = game->world_;
    if(mapinfo.savegame)
        mapinfo.savegame->sgd.ReadSnapshot(*game, *this);
    else
    {
        RTTR_Assert(mapinfo.type != MapType::Savegame);
        /// Startbündnisse setzen
        for(unsigned i = 0; i < gameWorld.GetNumPlayers(); ++i)
            gameWorld.GetPlayer(i).MakeStartPacts();

        MapLoader loader(gameWorld);
        if(!loader.Load(mapinfo.filepath)
           || (!mapinfo.luaFilepath.empty() && !loader.LoadLuaScript(*game, *this, mapinfo.luaFilepath)))
        {
            OnError(ClientError::InvalidMap);
            return;
        }
        gameWorld.SetupResources();

        if(game->ggs_.objective == GameObjective::EconomyMode)
        {
            unsigned int selection = game->ggs_.getSelection(AddonId::ECONOMY_MODE_GAME_LENGTH);
            gameWorld.setEconHandler(std::make_unique<EconomyModeHandler>(AddonEconomyModeGameLengthList[selection]
                                                                          / SPEED_GF_LENGTHS[referenceSpeed]));
        }
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
    RTTR_Assert(state == ClientState::Loading);

    state = ClientState::Loaded;

    if(replayMode)
        OnGameStart();
    else
    {
        // Notify server that we are ready
        if(IsHost())
        {
            for(unsigned id = 0; id < GetNumPlayers(); id++)
            {
                if(GetPlayer(id).ps == PlayerState::AI)
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
    RTTR_Assert(state == ClientState::Game || state == ClientState::Loaded || state == ClientState::Loading);
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
    if(state != ClientState::Connect)
        return true;
    // haben wir eine ungültige ID erhalten? (aka Server-Voll)
    if(msg.player == GameMessageWithPlayer::NO_PLAYER_ID)
    {
        OnError(ClientError::ServerFull);
        return true;
    }

    mainPlayer.playerId = msg.player;

    // Server-Typ senden
    mainPlayer.sendMsgAsync(new GameMessage_Server_Type(clientconfig.servertyp, rttr::version::GetRevision()));
    return true;
}

/**
 *  Player-List-Nachricht.
 */
bool GameClient::OnGameMessage(const GameMessage_Player_List& msg)
{
    if(state != ClientState::Connect && state != ClientState::Config)
        return true;
    RTTR_Assert(gameLobby);
    RTTR_Assert(gameLobby->getNumPlayers() == msg.playerInfos.size());
    if(gameLobby->getNumPlayers() != msg.playerInfos.size())
        return true;

    for(unsigned i = 0; i < gameLobby->getNumPlayers(); ++i)
        gameLobby->getPlayer(i) = msg.playerInfos[i];

    if(state != ClientState::Config)
    {
        state = ClientState::Config;
        if(ci)
            ci->CI_NextConnectState(ConnectState::Finished);
    }
    return true;
}

bool GameClient::OnGameMessage(const GameMessage_Player_Name& msg)
{
    if(state != ClientState::Config)
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
    if(state != ClientState::Config)
        return true;

    if(msg.player >= gameLobby->getNumPlayers())
        return true;

    JoinPlayerInfo& playerInfo = gameLobby->getPlayer(msg.player);

    playerInfo.name = msg.name;
    playerInfo.ps = PlayerState::Occupied;
    playerInfo.ping = 0;

    if(ci)
        ci->CI_NewPlayer(msg.player);
    return true;
}

bool GameClient::OnGameMessage(const GameMessage_Player_Ping& msg)
{
    if(state == ClientState::Config)
    {
        if(msg.player >= gameLobby->getNumPlayers())
            return true;
        gameLobby->getPlayer(msg.player).ping = msg.ping;
    } else if(state == ClientState::Loading || state == ClientState::Loaded || state == ClientState::Game)
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
    if(state != ClientState::Config)
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
    if(state != ClientState::Config)
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
    if(state != ClientState::Config)
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
    if(state != ClientState::Config)
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
    if(state != ClientState::Config)
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
    if(state == ClientState::Config)
    {
        if(msg.player >= gameLobby->getNumPlayers())
            return true;
        gameLobby->getPlayer(msg.player).ps = PlayerState::Free;
    } else if(state == ClientState::Loading || state == ClientState::Loaded || state == ClientState::Game)
    {
        // Im Spiel anzeigen, dass der Spieler das Spiel verlassen hat
        GamePlayer& player = GetPlayer(msg.player);
        if(player.ps != PlayerState::AI)
        {
            player.ps = PlayerState::AI;
            player.aiInfo = AI::Info(AI::Type::Dummy);
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

    if(state == ClientState::Config)
    {
        if(msg.player >= gameLobby->getNumPlayers() || msg.player2 >= gameLobby->getNumPlayers())
            return true;

        // During preparation just swap the players
        using std::swap;
        swap(gameLobby->getPlayer(msg.player), gameLobby->getPlayer(msg.player2));
        // Some things cannot be changed in savegames
        if(mapinfo.type == MapType::Savegame)
            gameLobby->getPlayer(msg.player).FixSwappedSaveSlot(gameLobby->getPlayer(msg.player2));

        // Evtl. sind wir betroffen?
        if(mainPlayer.playerId == msg.player)
            mainPlayer.playerId = msg.player2;
        else if(mainPlayer.playerId == msg.player2)
            mainPlayer.playerId = msg.player;

        if(ci)
            ci->CI_PlayersSwapped(msg.player, msg.player2);
    } else if(state == ClientState::Loading || state == ClientState::Loaded || state == ClientState::Game)
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
    if(state != ClientState::Connect)
        return true;

    switch(msg.err_code)
    {
        case 0: // ok
            break;

        default:
        case 1:
        {
            OnError(ClientError::InvalidServerType);
            return true;
        }
        break;

        case 2:
        {
            OnError(ClientError::WrongVersion);
            return true;
        }
        break;
    }

    mainPlayer.sendMsgAsync(new GameMessage_Server_Password(clientconfig.password));

    if(ci)
        ci->CI_NextConnectState(ConnectState::QueryPw);
    return true;
}

/**
 *  Server-Passwort-Nachricht.
 */
bool GameClient::OnGameMessage(const GameMessage_Server_Password& msg)
{
    if(state != ClientState::Connect)
        return true;

    if(msg.password != "true")
    {
        OnError(ClientError::WrongPassword);
        return true;
    }

    mainPlayer.sendMsgAsync(new GameMessage_Player_Name(0xFF, SETTINGS.lobby.name));
    mainPlayer.sendMsgAsync(new GameMessage_MapRequest(true));

    if(ci)
        ci->CI_NextConnectState(ConnectState::QueryMapName);
    return true;
}

/**
 *  Server-Name-Nachricht.
 */
bool GameClient::OnGameMessage(const GameMessage_Server_Name& msg)
{
    if(state != ClientState::Connect)
        return true;
    clientconfig.gameName = msg.name;

    if(ci)
        ci->CI_NextConnectState(ConnectState::QueryPlayerList);
    return true;
}

/**
 *  Server-Start-Nachricht
 */
bool GameClient::OnGameMessage(const GameMessage_Server_Start& msg)
{
    if(state != ClientState::Config)
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
    if(msg.destination == ChatDestination::System)
    {
        SystemChat(msg.text, (msg.player < game->world_.GetNumPlayers()) ? msg.player : GetPlayerId());
        return true;
    }
    if(state == ClientState::Game)
    {
        // Ingame message: Do some checking and logging
        if(msg.player >= game->world_.GetNumPlayers())
            return true;

        /// Mit im Replay aufzeichnen
        if(replayinfo && replayinfo->replay.IsRecording())
            replayinfo->replay.AddChatCommand(GetGFNumber(), msg.player, msg.destination, msg.text);

        const GamePlayer& player = game->world_.GetPlayer(msg.player);

        // Besiegte dürfen nicht mehr heimlich mit Verbüdeten oder Feinden reden
        if(player.IsDefeated() && msg.destination != ChatDestination::All)
            return true;

        const auto isValidRecipient = [&msg, &player](const unsigned playerId) {
            // Always send to self
            if(msg.player == playerId)
                return true;
            switch(msg.destination)
            {
                case ChatDestination::System:
                case ChatDestination::All: return true;
                case ChatDestination::Allies: return player.IsAlly(playerId);
                case ChatDestination::Enemies: return !player.IsAlly(playerId);
            }
            return true; // LCOV_EXCL_LINE
        };
        for(AIPlayer& ai : game->aiPlayers_)
        {
            if(isValidRecipient(ai.GetPlayerId()))
                ai.OnChatMessage(msg.player, msg.destination, msg.text);
        }

        if(!isValidRecipient(GetPlayerId()))
            return true;
    } else if(state == ClientState::Config)
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
    if(state != ClientState::Game)
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
    fileName += "_" + s25util::toStringClassic(GetPlayerId()) + "_";
    fileName += GetPlayer(GetPlayerId()).name;

    const bfs::path filePathSave = RTTRCONFIG.ExpandPath(s25::folders::save) / makePortableFileName(fileName + ".sav");
    const bfs::path filePathLog =
      RTTRCONFIG.ExpandPath(s25::folders::logs) / makePortableFileName(fileName + "Player.log");
    saveRandomLog(filePathLog, RANDOM.GetAsyncLog());
    SaveToFile(filePathSave);
    LOG.write(_("Async log saved at %1%,\ngame saved at %2%\n")) % filePathLog % filePathSave;
    return true;
}

/**
 *  Server-Countdown-Nachricht.
 */
bool GameClient::OnGameMessage(const GameMessage_Countdown& msg)
{
    if(state != ClientState::Config)
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
    if(state != ClientState::Config)
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
    if(state != ClientState::Connect)
        return true;

    // full path
    const std::string portFilename = makePortableFileName(msg.filename);
    if(portFilename.empty())
    {
        LOG.write("Invalid filename received!\n");
        OnError(ClientError::InvalidMap);
    }
    mapinfo.filepath = RTTRCONFIG.ExpandPath(s25::folders::mapsPlayed) / portFilename;
    mapinfo.type = msg.mt;

    // lua script file path
    if(msg.luaLen > 0)
        mapinfo.luaFilepath = bfs::path(mapinfo.filepath).replace_extension("lua");
    else
        mapinfo.luaFilepath.clear();

    if(bfs::exists(mapinfo.filepath) && (mapinfo.luaFilepath.empty() || bfs::exists(mapinfo.luaFilepath))
       && CreateLobby())
    {
        mapinfo.mapData.CompressFromFile(mapinfo.filepath, &mapinfo.mapChecksum);
        if(mapinfo.mapData.data.size() == msg.mapCompressedLen && mapinfo.mapData.uncompressedLength == msg.mapLen)
        {
            bool ok = true;
            if(!mapinfo.luaFilepath.empty())
            {
                mapinfo.luaData.CompressFromFile(mapinfo.luaFilepath, &mapinfo.luaChecksum);
                ok = (mapinfo.luaData.data.size() == msg.luaCompressedLen
                      && mapinfo.luaData.uncompressedLength == msg.luaLen);
            }

            if(ok)
            {
                mainPlayer.sendMsgAsync(new GameMessage_Map_Checksum(mapinfo.mapChecksum, mapinfo.luaChecksum));
                return true;
            }
        }
        gameLobby.reset();
    }
    mapinfo.mapData.uncompressedLength = msg.mapLen;
    mapinfo.luaData.uncompressedLength = msg.luaLen;
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
    if(state != ClientState::Connect)
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
            OnError(ClientError::MapTransmission);
            return true;
        }
        if(!mapinfo.luaFilepath.empty() && !mapinfo.luaData.DecompressToFile(mapinfo.luaFilepath, &mapinfo.luaChecksum))
        {
            OnError(ClientError::MapTransmission);
            return true;
        }
        RTTR_Assert(!mapinfo.luaFilepath.empty() || mapinfo.luaChecksum == 0);

        if(!CreateLobby())
        {
            OnError(ClientError::MapTransmission);
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
        case MapType::OldMap:
        {
            libsiedler2::Archiv map;

            // Karteninformationen laden
            if(libsiedler2::loader::LoadMAP(mapinfo.filepath, map, true) != 0)
            {
                LOG.write("GameClient::OnMapData: ERROR: Map %1%, couldn't load header!\n") % mapinfo.filepath;
                return false;
            }

            const libsiedler2::ArchivItem_Map_Header& header =
              checkedCast<const libsiedler2::ArchivItem_Map*>(map.get(0))->getHeader();
            numPlayers = header.getNumPlayers();
            mapinfo.title = s25util::ansiToUTF8(header.getName());
        }
        break;
        case MapType::Savegame:
            mapinfo.savegame = std::make_unique<Savegame>();
            if(!mapinfo.savegame->Load(mapinfo.filepath, SaveGameDataToLoad::HeaderAndSettings))
                return false;

            numPlayers = mapinfo.savegame->GetNumPlayers();
            mapinfo.title = mapinfo.savegame->GetMapName();
            break;
        default: return false;
    }

    if(GetPlayerId() >= numPlayers)
        return false;

    gameLobby = std::make_shared<GameLobby>(mapinfo.type == MapType::Savegame, IsHost(), numPlayers);
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// map-checksum
/// @param message  Nachricht, welche ausgeführt wird
bool GameClient::OnGameMessage(const GameMessage_Map_ChecksumOK& msg)
{
    if(state != ClientState::Connect)
        return true;
    LOG.writeToFile("<<< NMS_MAP_CHECKSUM(%d)\n") % (msg.correct ? 1 : 0);

    if(!msg.correct)
    {
        gameLobby.reset();
        if(msg.retryAllowed)
            mainPlayer.sendMsgAsync(new GameMessage_MapRequest(false));
        else
            OnError(ClientError::MapTransmission);
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// server typ
/// @param message  Nachricht, welche ausgeführt wird
bool GameClient::OnGameMessage(const GameMessage_GGSChange& msg)
{
    if(state != ClientState::Config)
        return true;
    LOG.writeToFile("<<< NMS_GGS_CHANGE\n");

    gameLobby->getSettings() = msg.ggs;

    if(ci)
        ci->CI_GGSChanged(msg.ggs);
    return true;
}

bool GameClient::OnGameMessage(const GameMessage_RemoveLua&)
{
    if(state != ClientState::Connect && state != ClientState::Config)
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
    if(state != ClientState::Game)
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
    if(state != ClientState::Game)
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
    const bool isSkipping = skiptogf > curGF;
    // Is it time for the next GF? If we are skipping, it is always time for the next GF
    if(isSkipping || (currentTime - framesinfo.lastTime) >= framesinfo.gf_length)
    {
        try
        {
            if(isSkipping)
            {
                // We are always in realtime
                framesinfo.lastTime = currentTime;
            } else
            {
                // Advance simulation time (lastTime) by 1 GF
                framesinfo.lastTime += framesinfo.gf_length;
            }
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
                    // If a player is lagging (we did not got his commands) "pause" the game by skipping the rest of
                    // this function
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
                        LOG.write("Client: Speed changed at %1% from %2% to %3% (NWF: %4%)\n") % curGF
                          % helpers::withUnit(oldGFLen) % helpers::withUnit(framesinfo.gf_length)
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

        } catch(LuaExecutionError& e)
        {
            if(ci)
            {
                SystemChat(
                  (boost::format(_("Error during execution of lua script: %1\nGame stopped!")) % e.what()).str());
                ci->CI_Error(ClientError::InvalidMap);
            }
            Stop();
        }
        if(skiptogf == GetGFNumber())
            skiptogf = 0;
    }
    framesinfo.frameTime = std::chrono::duration_cast<FramesInfo::milliseconds32_t>(currentTime - framesinfo.lastTime);
    // Check remaining time until next GF
    if(framesinfo.frameTime >= framesinfo.gf_length)
    {
        // This can happen, if we don't call this method in intervalls less than gf_length or gf_length has changed
        // TODO: Run multiple GFs per call.
        // For now just make sure it is less than gf_length by skipping some simulation time,
        // until we are only a bit less than 1 GF behind
        // However we allow the simulation to lack behind for a few frames, so if there was a single spike we can still
        // catch up in the next visual frames
        using DurationType = decltype(framesinfo.gf_length);
        constexpr auto maxLackFrames = 5;

        RTTR_Assert(framesinfo.gf_length > DurationType::zero());
        const auto maxFrameTime = framesinfo.gf_length - DurationType(1);

        if(framesinfo.frameTime > maxLackFrames * framesinfo.gf_length)
            framesinfo.lastTime += framesinfo.frameTime - maxFrameTime; // Skip simulation time until caught up
        framesinfo.frameTime = maxFrameTime;
    }
    // This is assumed by drawing code for interpolation
    RTTR_Assert(framesinfo.frameTime < framesinfo.gf_length);
}

void GameClient::HandleAutosave()
{
    // If inactive or during replay -> no autosave
    if(!SETTINGS.interface.autosave_interval || replayMode)
        return;

    // Alle .... GF
    if(GetGFNumber() % SETTINGS.interface.autosave_interval == 0)
    {
        std::string filename;
        if(mapinfo.title.empty())
            filename = std::string(_("Auto-Save")) + ".sav";
        else
            filename = mapinfo.title + " (" + _("Auto-Save") + ").sav";

        SaveToFile(RTTRCONFIG.ExpandPath(s25::folders::save) / filename);
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
    mainPlayer.sendMsgAsync(
      new GameMessage_GameCommand(player, AsyncChecksum::create(*game), std::vector<gc::GameCommandPtr>()));
}

void GameClient::WritePlayerInfo(SavedFile& file)
{
    RTTR_Assert(state == ClientState::Loading || state == ClientState::Loaded || state == ClientState::Game);
    // Spielerdaten
    for(unsigned i = 0; i < GetNumPlayers(); ++i)
        file.AddPlayer(GetPlayer(i));
}

void GameClient::OnGameStart()
{
    if(state == ClientState::Loaded)
    {
        GAMEMANAGER.ResetAverageGFPS();
        framesinfo.lastTime = FramesInfo::UsedClock::now();
        state = ClientState::Game;
        if(ci)
            ci->CI_GameStarted();
    } else if(state == ClientState::Game && !game->IsStarted())
    {
        framesinfo.isPaused = replayMode;
        game->Start(!!mapinfo.savegame);
    }
}

void GameClient::StartReplayRecording(const unsigned random_init)
{
    replayinfo = std::make_unique<ReplayInfo>();
    replayinfo->filename = s25util::Time::FormatTime("%Y-%m-%d_%H-%i-%s") + ".rpl";
    replayinfo->replay.random_init = random_init;

    WritePlayerInfo(replayinfo->replay);
    replayinfo->replay.ggs = game->ggs_;

    // Datei speichern
    if(!replayinfo->replay.StartRecording(RTTRCONFIG.ExpandPath(s25::folders::replays) / replayinfo->filename, mapinfo))
    {
        LOG.write(_("Replayfile couldn't be opened. No replay will be recorded\n"));
        replayinfo.reset();
    }
}

bool GameClient::StartReplay(const boost::filesystem::path& path)
{
    RTTR_Assert(state == ClientState::Stopped);
    mapinfo.Clear();
    replayinfo = std::make_unique<ReplayInfo>();

    if(!replayinfo->replay.LoadHeader(path) || !replayinfo->replay.LoadGameData(mapinfo)) //-V807
    {
        LOG.write(_("Invalid Replay %1%! Reason: %2%\n")) % path
          % (replayinfo->replay.GetLastErrorMsg().empty() ? _("Unknown") : replayinfo->replay.GetLastErrorMsg());
        OnError(ClientError::InvalidMap);
        replayinfo.reset();
        return false;
    }
    replayinfo->filename = path.filename();

    gameLobby = std::make_shared<GameLobby>(true, true, replayinfo->replay.GetNumPlayers());

    for(unsigned i = 0; i < replayinfo->replay.GetNumPlayers(); ++i)
        gameLobby->getPlayer(i) = JoinPlayerInfo(replayinfo->replay.GetPlayer(i));

    bool playerFound = false;
    // Find a player to spectate from
    // First find a human player
    for(unsigned char i = 0; i < gameLobby->getNumPlayers(); ++i)
    {
        if(gameLobby->getPlayer(i).ps == PlayerState::Occupied)
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
            if(gameLobby->getPlayer(i).ps == PlayerState::AI)
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
        case MapType::OldMap:
        {
            // Richtigen Pfad zur Map erstellen
            bfs::path mapFilePath = RTTRCONFIG.ExpandPath(s25::folders::mapsPlayed) / mapinfo.filepath.filename();
            mapinfo.filepath = mapFilePath;
            if(!mapinfo.mapData.DecompressToFile(mapinfo.filepath))
            {
                LOG.write(_("Error decompressing map file"));
                OnError(ClientError::MapTransmission);
                return false;
            }
            if(mapinfo.luaData.uncompressedLength)
            {
                mapinfo.luaFilepath = mapFilePath.replace_extension("lua");
                if(!mapinfo.luaData.DecompressToFile(mapinfo.luaFilepath))
                {
                    LOG.write(_("Error decompressing lua file"));
                    OnError(ClientError::MapTransmission);
                    return false;
                }
            }
        }
        break;
        case MapType::Savegame: break;
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
        OnError(ClientError::InvalidMap);
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
    const unsigned currenttime = std::chrono::duration_cast<FramesInfo::milliseconds32_t>(
                                   (framesinfo.lastTime + framesinfo.frameTime).time_since_epoch())
                                   .count();
    return ((currenttime % unit) * max / unit + offset) % max;
}

unsigned GameClient::Interpolate(unsigned max_val, const GameEvent* ev)
{
    RTTR_Assert(ev);
    // TODO: Move to some animation system that is part of game
    FramesInfo::milliseconds32_t elapsedTime;
    if(state == ClientState::Game)
        elapsedTime = (GetGFNumber() - ev->startGF) * framesinfo.gf_length + framesinfo.frameTime;
    else
        elapsedTime = FramesInfo::milliseconds32_t::zero();
    FramesInfo::milliseconds32_t duration = ev->length * framesinfo.gf_length;
    return helpers::interpolate(0u, max_val, elapsedTime, duration);
}

int GameClient::Interpolate(int x1, int x2, const GameEvent* ev)
{
    RTTR_Assert(ev);
    FramesInfo::milliseconds32_t elapsedTime;
    if(state == ClientState::Game)
        elapsedTime = (GetGFNumber() - ev->startGF) * framesinfo.gf_length + framesinfo.frameTime;
    else
        elapsedTime = FramesInfo::milliseconds32_t::zero();
    FramesInfo::milliseconds32_t duration = ev->length * framesinfo.gf_length;
    return helpers::interpolate(x1, x2, elapsedTime, duration);
}

void GameClient::ServerLost()
{
    OnError(ClientError::ConnectionLost);
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
            road.mode = RoadBuildMode::Disabled;

            // spiel aktualisieren
            gwv.Draw(road, MapPoint::Invalid(), false);

            // text oben noch hinschreiben
            boost::format nwfString(_("current GF: %u - still fast forwarding: %d GFs left (%d %%)"));
            nwfString % GetGFNumber() % (gf - i) % (i * 100 / gf);
            LargeFont->Draw(DrawPoint(VIDEODRIVER.GetRenderSize() / 2u), nwfString.str(), FontStyle::CENTER,
                            COLOR_YELLOW);

            VIDEODRIVER.SwapBuffers();
        }
        ExecuteGameFrame();
    }

    // Spiel pausieren & text ausgabe wie lang das jetzt gedauert hat
    unsigned ticks = VIDEODRIVER.GetTickCount() - start_ticks;
    boost::format text(_("Jump finished (%1$.3g seconds)."));
    text % (ticks / 1000.0);
    SystemChat(text.str());
    SetPause(true);
}

void GameClient::SystemChat(const std::string& text)
{
    SystemChat(text, GetPlayerId());
}

void GameClient::SystemChat(const std::string& text, unsigned char fromPlayerIdx)
{
    if(ci)
        ci->CI_Chat(fromPlayerIdx, ChatDestination::System, text);
}

bool GameClient::SaveToFile(const boost::filesystem::path& filepath)
{
    mainPlayer.sendMsg(GameMessage_Chat(GetPlayerId(), ChatDestination::System, "Saving game..."));

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
        save.sgd.MakeSnapshot(*game);
        // Und alles speichern
        return save.Save(filepath, mapinfo.title);
    } catch(std::exception& e)
    {
        SystemChat(std::string("Error during saving: ") + e.what());
        return false;
    }
}

void GameClient::ResetVisualSettings()
{
    GetPlayer(GetPlayerId()).FillVisualSettings(visual_settings);
}

void GameClient::SetPause(bool pause)
{
    if(state == ClientState::Stopped)
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
        auto* msg = new GameMessage_Pause(pause);
        if(pause)
            OnGameMessage(*msg);
        mainPlayer.sendMsgAsync(msg);
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
    if(framesinfo.isPaused || GetPlayer(GetPlayerId()).IsDefeated() || IsReplayModeOn())
        return false;

    gameCommands_.push_back(gc);
    return true;
}

unsigned GameClient::GetNumPlayers() const
{
    RTTR_Assert(state == ClientState::Loading || state == ClientState::Loaded || state == ClientState::Game);
    return game->world_.GetNumPlayers();
}

GamePlayer& GameClient::GetPlayer(const unsigned id)
{
    RTTR_Assert(state == ClientState::Loading || state == ClientState::Loaded || state == ClientState::Game);
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
    seconds numSeconds = duration_cast<seconds>(gf * SPEED_GF_LENGTHS[referenceSpeed]);

    // Angaben rausfiltern
    hours numHours = duration_cast<hours>(numSeconds);
    numSeconds -= numHours;
    minutes numMinutes = duration_cast<minutes>(numSeconds);
    numSeconds -= numMinutes;

    // ganze Stunden mit dabei? Dann entsprechend anderes format, ansonsten ignorieren wir die einfach
    if(numHours.count())
        return helpers::format("%u:%02u:%02u", numHours.count(), numMinutes.count(), numSeconds.count());
    else
        return helpers::format("%02u:%02u", numMinutes.count(), numSeconds.count());
}

const boost::filesystem::path& GameClient::GetReplayFilename() const
{
    static boost::filesystem::path emptyString;
    return replayinfo ? replayinfo->filename : emptyString;
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
    if(game && rttr::enum_cast(game->ggs_.objective) >= rttr::enum_cast(GameObjective::Tournament1)
       && static_cast<unsigned>(rttr::enum_cast(game->ggs_.objective))
            < rttr::enum_cast(GameObjective::Tournament1) + NUM_TOURNAMENT_MODES)
    {
        const auto turnamentMode = rttr::enum_cast(game->ggs_.objective) - rttr::enum_cast(GameObjective::Tournament1);
        return minutes(TOURNAMENT_MODES_DURATION[turnamentMode]) / SPEED_GF_LENGTHS[referenceSpeed];
    } else
        return 0;
}

void GameClient::ToggleHumanAIPlayer()
{
    RTTR_Assert(!IsReplayModeOn());
    auto it = helpers::find_if(game->aiPlayers_,
                               [id = this->GetPlayerId()](const auto& player) { return player.GetPlayerId() == id; });
    if(it != game->aiPlayers_.end())
        game->aiPlayers_.erase(it);
    else
        game->AddAIPlayer(CreateAIPlayer(GetPlayerId(), AI::Info(AI::Type::Default, AI::Level::Easy)));
}

void GameClient::RequestSwapToPlayer(const unsigned char newId)
{
    if(state != ClientState::Game)
        return;
    GamePlayer& player = GetPlayer(newId);
    if(player.ps == PlayerState::AI && player.aiInfo.type == AI::Type::Dummy)
        mainPlayer.sendMsgAsync(new GameMessage_Player_Swap(0xFF, newId));
}
