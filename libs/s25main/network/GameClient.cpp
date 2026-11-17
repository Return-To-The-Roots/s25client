// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
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
#include "GameSetup.h"
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
#include "ai/AIPlayer.h"
#include "ai/random.h"
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
#include "gameTypes/RoadBuildState.h"
#include "gameData/GameConsts.h"
#include "gameData/PortraitConsts.h"
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

namespace {
constexpr bool DEBUG_MODE =
#ifndef NDEBUG
  true;
#else
  false;
#endif
void copyFileIfPathDifferent(const boost::filesystem::path& src_path, const boost::filesystem::path& dst_path)
{
    if(src_path != dst_path)
    {
        boost::system::error_code ignoredEc;
        constexpr auto overwrite_existing =
#if BOOST_VERSION >= 107400
          boost::filesystem::copy_options::overwrite_existing;
#else
          boost::filesystem::copy_option::overwrite_if_exists;
#endif
        copy_file(src_path, dst_path, overwrite_existing, ignoredEc);
    }
}
} // namespace

void GameClient::ClientConfig::clear()
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
    stop();
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
bool GameClient::connect(const std::string& server, const std::string& password, ServerType servertyp,
                         unsigned short port, bool host, bool use_ipv6)
{
    stop();

    RTTR_Assert(aiBattlePlayers_.empty());

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
            GAMESERVER.stop();
        return false;
    }

    state = ClientState::Connect;
    advanceState(ConnectState::Initiated);

    // Es wird kein Replay abgespielt, sondern dies ist ein richtiges Spiel
    replayMode = false;

    return true;
}

bool GameClient::hostGame(const CreateServerInfo& csi, const MapDescription& map)
{
    std::string hostPw = createRandString(20);
    // Copy the map and lua to the played map folders to avoid having to transmit it from the (local) server
    const auto playedMapPath = RTTRCONFIG.expandPath(s25::folders::mapsPlayed) / map.map_path.filename();
    copyFileIfPathDifferent(map.map_path, playedMapPath);
    if(map.lua_path)
    {
        const auto playedMapLuaPath = RTTRCONFIG.expandPath(s25::folders::mapsPlayed) / map.lua_path->filename();
        copyFileIfPathDifferent(*map.lua_path, playedMapLuaPath);
    }
    return GAMESERVER.start(csi, map, hostPw) && connect("localhost", hostPw, csi.type, csi.port, true, csi.ipv6);
}

/**
 *  Hauptschleife des Clients
 */
void GameClient::run()
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
            serverLost();
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
            serverLost();
        }
    }

    if(state == ClientState::Loaded)
    {
        // All players ready?
        if(nwfInfo->isReady())
            onGameStart();
    } else if(state == ClientState::Game)
        executeGameFrame();

    // maximal 10 Pakete verschicken
    mainPlayer.sendMsgs(10);

    mainPlayer.executeMsgs(*this);
}

/**
 *  Stoppt das Spiel
 */
void GameClient::stop()
{
    if(state == ClientState::Stopped)
        return;

    if(game)
        exitGame();
    else if(state == ClientState::Connect || state == ClientState::Config)
        gameLobby.reset();

    if(isHost())
        GAMESERVER.stop();

    framesinfo.clear();
    clientconfig.clear();
    mapinfo.clear();

    if(replayinfo)
    {
        if(replayinfo->replay.isRecording())
            replayinfo->replay.stopRecording();
        replayinfo->replay.close();
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

    aiBattlePlayers_.clear();
}

std::shared_ptr<GameLobby> GameClient::getGameLobby()
{
    RTTR_Assert(state != ClientState::Config || gameLobby);
    return gameLobby;
}

const AIPlayer* GameClient::getAIPlayer(unsigned id) const
{
    if(!game)
        return nullptr;
    return game->getAIPlayer(id);
}

/**
 *  Startet ein Spiel oder Replay.
 *
 *  @param[in] random_init Initialwert des Zufallsgenerators.
 */
void GameClient::startGame(const unsigned random_init)
{
    RTTR_Assert(state == ClientState::Config || (state == ClientState::Stopped && replayMode));

    // Mond malen
    Position moonPos = VIDEODRIVER.getMousePos();
    moonPos.y -= 40;
    LOADER.getImageN("resource", 33)->drawFull(moonPos);
    VIDEODRIVER.swapBuffers();

    // Start in pause mode
    framesinfo.isPaused = true;

    // Je nach Geschwindigkeit GF-Länge einstellen
    framesinfo.gf_length = SPEED_GF_LENGTHS[gameLobby->getSettings().speed];
    framesinfo.gfLengthReq = framesinfo.gf_length;

    // Random-Generator initialisieren
    RANDOM.init(random_init);
    AI::getRandomGenerator().seed(random_init);

    if(!isReplayModeOn() && mapinfo.savegame && !mapinfo.savegame->load(mapinfo.filepath, SaveGameDataToLoad::All))
    {
        onError(ClientError::InvalidMap);
        return;
    }

    // If we have a savegame, start at its first GF, else at 0
    unsigned startGF = (mapinfo.type == MapType::Savegame) ? mapinfo.savegame->start_gf : 0;
    // Create the game
    game =
      std::make_shared<Game>(std::move(gameLobby->getSettings()), startGF,
                             std::vector<PlayerInfo>(gameLobby->getPlayers().begin(), gameLobby->getPlayers().end()));
    if(!isReplayModeOn())
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
    getPlayer(getPlayerId()).fillVisualSettings(default_settings);

    try
    {
        setupGameWorld(*game, mapinfo, *this, getReplay());
    } catch(const GameSetupError& error)
    {
        LOG.write("%1%\n") % error.what();
        onError(ClientError::InvalidMap);
        return;
    }

    // Update visual settings
    resetVisualSettings();

    if(!replayMode)
    {
        RTTR_Assert(!replayinfo);
        startReplayRecording(random_init);
    }

    // Daten nach dem Schreiben des Replays ggf wieder löschen
    mapinfo.mapData.clear();
}

void GameClient::gameLoaded()
{
    RTTR_Assert(state == ClientState::Loading);

    state = ClientState::Loaded;

    if(replayMode)
        onGameStart();
    else
    {
        // Notify server that we are ready
        if(isHost())
        {
            for(unsigned id = 0; id < getNumPlayers(); id++)
            {
                if(getPlayer(id).ps == PlayerState::AI)
                {
                    game->addAIPlayer(createAIPlayer(id, getPlayer(id).aiInfo));
                    sendNothingNC(id);
                }
            }
            if(isAIBattleModeOn())
                toggleHumanAIPlayer(aiBattlePlayers_[getPlayerId()]);
        }
        sendNothingNC();
    }
}

void GameClient::exitGame()
{
    RTTR_Assert(state == ClientState::Game || state == ClientState::Loaded || state == ClientState::Loading);
    game.reset();
    nwfInfo.reset();
    // Clear remaining commands
    gameCommands_.clear();
}

unsigned GameClient::getGFNumber() const
{
    return game->em_->getCurrentGF();
}

/**
 *  Ping-Nachricht.
 */
bool GameClient::onGameMessage(const GameMessage_Ping& /*msg*/)
{
    mainPlayer.sendMsgAsync(new GameMessage_Pong());
    return true;
}

/**
 *  Player-ID-Nachricht.
 */
bool GameClient::onGameMessage(const GameMessage_Player_Id& msg)
{
    if(!verifyState(ConnectState::Initiated))
        return true;
    // haben wir eine ungültige ID erhalten? (aka Server-Voll)
    if(msg.player == GameMessageWithPlayer::NO_PLAYER_ID)
    {
        onError(ClientError::ServerFull);
        return true;
    }

    mainPlayer.playerId = msg.player;

    // Server-Typ senden
    mainPlayer.sendMsgAsync(new GameMessage_Server_Type(clientconfig.servertyp, rttr::version::getRevision()));
    advanceState(ConnectState::VerifyServer);
    return true;
}

/**
 *  Player-List-Nachricht.
 */
bool GameClient::onGameMessage(const GameMessage_Player_List& msg)
{
    if(state != ClientState::Config && !verifyState(ConnectState::QueryPlayerList))
        return true;
    RTTR_Assert(gameLobby);
    RTTR_Assert(gameLobby->getNumPlayers() == msg.playerInfos.size());
    if(gameLobby->getNumPlayers() != msg.playerInfos.size())
    {
        onError(ClientError::InvalidMessage);
        return true;
    }

    for(unsigned i = 0; i < gameLobby->getNumPlayers(); ++i)
        gameLobby->getPlayer(i) = msg.playerInfos[i];

    if(state == ClientState::Connect)
        advanceState(ConnectState::QuerySettings);
    return true;
}

bool GameClient::onGameMessage(const GameMessage_Player_Name& msg)
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

bool GameClient::onGameMessage(const GameMessage_Player_Portrait& msg)
{
    if(state != ClientState::Config)
        return true;
    if(msg.player >= gameLobby->getNumPlayers())
        return true;
    if(msg.playerPortraitIndex >= Portraits.size())
        return true;
    gameLobby->getPlayer(msg.player).portraitIndex = msg.playerPortraitIndex;
    if(ci)
        ci->CI_PlayerDataChanged(msg.player);
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// player joined
/// @param message  Nachricht, welche ausgeführt wird
bool GameClient::onGameMessage(const GameMessage_Player_New& msg)
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

bool GameClient::onGameMessage(const GameMessage_Player_Ping& msg)
{
    if(state == ClientState::Config)
    {
        if(msg.player >= gameLobby->getNumPlayers())
            return true;
        gameLobby->getPlayer(msg.player).ping = msg.ping;
    } else if(state == ClientState::Loading || state == ClientState::Loaded || state == ClientState::Game)
    {
        if(msg.player >= getNumPlayers())
            return true;
        getPlayer(msg.player).ping = msg.ping;
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
bool GameClient::onGameMessage(const GameMessage_Player_State& msg)
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
bool GameClient::onGameMessage(const GameMessage_Player_Nation& msg)
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
bool GameClient::onGameMessage(const GameMessage_Player_Team& msg)
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
bool GameClient::onGameMessage(const GameMessage_Player_Color& msg)
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
bool GameClient::onGameMessage(const GameMessage_Player_Ready& msg)
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
bool GameClient::onGameMessage(const GameMessage_Player_Kicked& msg)
{
    if(state == ClientState::Config)
    {
        if(msg.player >= gameLobby->getNumPlayers())
            return true;
        gameLobby->getPlayer(msg.player).ps = PlayerState::Free;
    } else if(state == ClientState::Loading || state == ClientState::Loaded || state == ClientState::Game)
    {
        // Im Spiel anzeigen, dass der Spieler das Spiel verlassen hat
        GamePlayer& player = getPlayer(msg.player);
        if(player.ps != PlayerState::AI)
        {
            player.ps = PlayerState::AI;
            player.aiInfo = AI::Info(AI::Type::Dummy);
            // Host has to handle it
            if(isHost())
            {
                game->addAIPlayer(createAIPlayer(msg.player, player.aiInfo));
                sendNothingNC(msg.player);
            }
        }
    } else
        return true;

    if(ci)
        ci->CI_PlayerLeft(msg.player);
    return true;
}

bool GameClient::onGameMessage(const GameMessage_Player_Swap& msg)
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
            gameLobby->getPlayer(msg.player).fixSwappedSaveSlot(gameLobby->getPlayer(msg.player2));

        // Evtl. sind wir betroffen?
        if(mainPlayer.playerId == msg.player)
            mainPlayer.playerId = msg.player2;
        else if(mainPlayer.playerId == msg.player2)
            mainPlayer.playerId = msg.player;

        if(ci)
            ci->CI_PlayersSwapped(msg.player, msg.player2);
    } else if(state == ClientState::Loading || state == ClientState::Loaded || state == ClientState::Game)
        changePlayerIngame(msg.player, msg.player2);
    else
        return true;
    mainPlayer.sendMsgAsync(new GameMessage_Player_SwapConfirm(msg.player, msg.player2));
    return true;
}

/**
 *  Server-Typ-Nachricht.
 */
bool GameClient::onGameMessage(const GameMessage_Server_TypeOK& msg)
{
    if(!verifyState(ConnectState::VerifyServer))
        return true;

    using StatusCode = GameMessage_Server_TypeOK::StatusCode;
    switch(msg.err_code)
    {
        case StatusCode::Ok: break;

        default:
        case StatusCode::InvalidServerType:
        {
            onError(ClientError::InvalidServerType);
            return true;
        }
        break;

        case StatusCode::WrongVersion:
        {
            LOG.write(_("Version mismatch. Server version: %1%, your version %2%")) % msg.version
              % rttr::version::getRevision();
            onError(ClientError::WrongVersion);
            return true;
        }
        break;
    }

    mainPlayer.sendMsgAsync(new GameMessage_Server_Password(clientconfig.password));

    advanceState(ConnectState::QueryPw);
    return true;
}

/**
 *  Server-Passwort-Nachricht.
 */
bool GameClient::onGameMessage(const GameMessage_Server_Password& msg)
{
    if(!verifyState(ConnectState::QueryPw))
        return true;

    if(msg.password != "true")
    {
        onError(ClientError::WrongPassword);
        return true;
    }

    mainPlayer.sendMsgAsync(new GameMessage_Player_Name(0xFF, SETTINGS.lobby.name));
    mainPlayer.sendMsgAsync(new GameMessage_Player_Portrait(0xFF, SETTINGS.lobby.portraitIndex));
    mainPlayer.sendMsgAsync(new GameMessage_MapRequest(true));

    advanceState(ConnectState::QueryMapInfo);
    return true;
}

/**
 *  Server-Name-Nachricht.
 */
bool GameClient::onGameMessage(const GameMessage_Server_Name& msg)
{
    if(!verifyState(ConnectState::QueryServerName))
        return true;
    clientconfig.gameName = msg.name;

    advanceState(ConnectState::QueryPlayerList);
    return true;
}

/**
 *  Server-Start-Nachricht
 */
bool GameClient::onGameMessage(const GameMessage_Server_Start& msg)
{
    if(state != ClientState::Config)
        return true;

    nwfInfo = std::make_shared<NWFInfo>();
    nwfInfo->init(msg.firstNwf, msg.cmdDelay);
    try
    {
        startGame(msg.random_init);
    } catch(const SerializedGameData::Error& error)
    {
        LOG.write("Error when loading game: %s\n") % error.what();
        stop();
        GAMEMANAGER.showMenu();
    }
    return true;
}

/**
 *  Server-Chat-Nachricht.
 */
bool GameClient::onGameMessage(const GameMessage_Chat& msg)
{
    if(msg.destination == ChatDestination::System)
    {
        systemChat(msg.text, (msg.player < game->world_.getNumPlayers()) ? msg.player : getPlayerId());
        return true;
    }
    if(state == ClientState::Game)
    {
        // Ingame message: Do some checking and logging
        if(msg.player >= game->world_.getNumPlayers())
            return true;

        /// Mit im Replay aufzeichnen
        if(replayinfo && replayinfo->replay.isRecording())
            replayinfo->replay.addChatCommand(getGFNumber(), msg.player, msg.destination, msg.text);

        const GamePlayer& player = game->world_.getPlayer(msg.player);

        // Besiegte dürfen nicht mehr heimlich mit Verbüdeten oder Feinden reden
        if(player.isDefeated() && msg.destination != ChatDestination::All)
            return true;

        const auto isValidRecipient = [&msg, &player](const unsigned playerId) {
            // Always send to self
            if(msg.player == playerId)
                return true;
            switch(msg.destination)
            {
                case ChatDestination::System:
                case ChatDestination::All: return true;
                case ChatDestination::Allies: return player.isAlly(playerId);
                case ChatDestination::Enemies: return !player.isAlly(playerId);
            }
            return true; // LCOV_EXCL_LINE
        };
        for(AIPlayer& ai : game->aiPlayers_)
        {
            if(isValidRecipient(ai.getPlayerId()))
                ai.onChatMessage(msg.player, msg.destination, msg.text);
        }

        if(!isValidRecipient(getPlayerId()))
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
bool GameClient::onGameMessage(const GameMessage_Server_Async& msg)
{
    if(state != ClientState::Game)
        return true;

    // Liste mit Namen und Checksummen erzeugen
    std::stringstream checksum_list;
    for(unsigned i = 0; i < msg.checksums.size(); ++i)
    {
        checksum_list << getPlayer(i).name << ": " << msg.checksums[i];
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
    fileName += "_" + s25util::toStringClassic(getPlayerId()) + "_";
    fileName += getPlayer(getPlayerId()).name;

    const bfs::path filePathSave = RTTRCONFIG.expandPath(s25::folders::save) / makePortableFileName(fileName + ".sav");
    const bfs::path filePathLog =
      RTTRCONFIG.expandPath(s25::folders::logs) / makePortableFileName(fileName + "Player.log");
    saveRandomLog(filePathLog, RANDOM.getAsyncLog());
    saveToFile(filePathSave);
    LOG.write(_("Async log saved at %1%,\ngame saved at %2%\n")) % filePathLog % filePathSave;
    return true;
}

/**
 *  Server-Countdown-Nachricht.
 */
bool GameClient::onGameMessage(const GameMessage_Countdown& msg)
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
bool GameClient::onGameMessage(const GameMessage_CancelCountdown& msg)
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
bool GameClient::onGameMessage(const GameMessage_Map_Info& msg)
{
    if(!verifyState(ConnectState::QueryMapInfo))
        return true;

    // For local savegame loads the filename comes from our own server and is already a valid
    // filesystem name — sanitizing it (spaces -> underscores) would create a renamed duplicate
    // alongside the original save file.
    const std::string portFilename = (clientconfig.servertyp == ServerType::Local && msg.mt == MapType::Savegame) ?
                                       msg.filename :
                                       makePortableFileName(msg.filename);
    if(portFilename.empty())
    {
        LOG.write("Invalid filename received!\n");
        onError(ClientError::InvalidMap);
        return true;
    }
    if(!MapInfo::verifySize(msg.mapLen, msg.luaLen, msg.mapCompressedLen, msg.luaCompressedLen))
    {
        onError(ClientError::InvalidMap);
        return true;
    }
    const auto targetPath =
      RTTRCONFIG.expandPath((msg.mt == MapType::Savegame) ? s25::folders::save : s25::folders::mapsPlayed);
    bfs::create_directories(targetPath);
    mapinfo.filepath = targetPath / portFilename;
    mapinfo.type = msg.mt;

    // lua script file path
    if(msg.luaLen > 0)
        mapinfo.luaFilepath = bfs::path(mapinfo.filepath).replace_extension("lua");
    else
        mapinfo.luaFilepath.clear();

    // We have the map locally already, so prepare and ask if this is the same as the one on the server
    if(bfs::exists(mapinfo.filepath) && (mapinfo.luaFilepath.empty() || bfs::exists(mapinfo.luaFilepath))
       && createLobby())
    {
        mapinfo.mapData.compressFromFile(mapinfo.filepath, &mapinfo.mapChecksum);
        if(mapinfo.mapData.data.size() == msg.mapCompressedLen && mapinfo.mapData.uncompressedLength == msg.mapLen)
        {
            bool ok = true;
            if(!mapinfo.luaFilepath.empty())
            {
                mapinfo.luaData.compressFromFile(mapinfo.luaFilepath, &mapinfo.luaChecksum);
                ok = (mapinfo.luaData.data.size() == msg.luaCompressedLen
                      && mapinfo.luaData.uncompressedLength == msg.luaLen);
            }

            if(ok)
            {
                mainPlayer.sendMsgAsync(new GameMessage_Map_Checksum(mapinfo.mapChecksum, mapinfo.luaChecksum));
                advanceState(ConnectState::VerifyMap);
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
    advanceState(ConnectState::ReceiveMap);
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// Kartendaten
/// @param message  Nachricht, welche ausgeführt wird
bool GameClient::onGameMessage(const GameMessage_Map_Data& msg)
{
    if(!verifyState(ConnectState::ReceiveMap))
        return true;

    LOG.writeToFile("<<< NMS_MAP_DATA(%u)\n") % msg.data.size();
    std::vector<char>& targetData = (msg.isMapData) ? mapinfo.mapData.data : mapinfo.luaData.data;
    if(msg.data.size() > targetData.size() || msg.offset > targetData.size() - msg.data.size())
    {
        onError(ClientError::MapTransmission);
        return true;
    }
    std::copy(msg.data.begin(), msg.data.end(), targetData.begin() + msg.offset);

    uint32_t totalSize = mapinfo.mapData.data.size();
    uint32_t receivedSize = msg.offset + msg.data.size();
    if(!mapinfo.luaFilepath.empty())
    {
        totalSize += mapinfo.luaData.data.size();
        // Assumes lua data comes after the map data
        if(!msg.isMapData)
            receivedSize += mapinfo.mapData.data.size();
    }
    if(ci)
        ci->CI_MapPartReceived(receivedSize, totalSize);

    if(receivedSize == totalSize)
    {
        if(!mapinfo.mapData.decompressToFile(mapinfo.filepath, &mapinfo.mapChecksum))
        {
            onError(ClientError::MapTransmission);
            return true;
        }
        if(!mapinfo.luaFilepath.empty() && !mapinfo.luaData.decompressToFile(mapinfo.luaFilepath, &mapinfo.luaChecksum))
        {
            onError(ClientError::MapTransmission);
            return true;
        }
        RTTR_Assert(!mapinfo.luaFilepath.empty() || mapinfo.luaChecksum == 0);

        if(!createLobby())
        {
            onError(ClientError::MapTransmission);
            return true;
        }

        mainPlayer.sendMsgAsync(new GameMessage_Map_Checksum(mapinfo.mapChecksum, mapinfo.luaChecksum));
        advanceState(ConnectState::VerifyMap);
    }
    return true;
}

bool GameClient::onGameMessage(const GameMessage_SkipToGF& msg)
{
    skiptogf = msg.targetGF;
    LOG.write("Jumping from GF %1% to GF %2%\n") % getGFNumber() % skiptogf;
    return true;
}

void GameClient::onError(ClientError error)
{
    if(ci)
        ci->CI_Error(error);
    stop();
}

void GameClient::advanceState(ConnectState newState)
{
    connectState = newState;
    if(ci)
        ci->CI_NextConnectState(connectState);
}

bool GameClient::verifyState(ConnectState expectedState)
{
    if(state != ClientState::Connect || connectState != expectedState)
    {
        onError(ClientError::InvalidMessage);
        return false;
    }
    return true;
}

bool GameClient::createLobby()
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
            if(!mapinfo.savegame->load(mapinfo.filepath, SaveGameDataToLoad::HeaderAndSettings))
                return false;

            numPlayers = mapinfo.savegame->getNumPlayers();
            mapinfo.title = mapinfo.savegame->getMapName();
            break;
        default: return false;
    }

    if(getPlayerId() >= numPlayers)
        return false;

    gameLobby = std::make_shared<GameLobby>(mapinfo.type == MapType::Savegame, isHost(), numPlayers);
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// map-checksum
/// @param message  Nachricht, welche ausgeführt wird
bool GameClient::onGameMessage(const GameMessage_Map_ChecksumOK& msg)
{
    if(!verifyState(ConnectState::VerifyMap))
        return true;
    LOG.writeToFile("<<< NMS_MAP_CHECKSUM(%d)\n") % (msg.correct ? 1 : 0);

    if(msg.correct)
        advanceState(ConnectState::QueryServerName);
    else
    {
        gameLobby.reset();
        if(msg.retryAllowed)
        {
            mainPlayer.sendMsgAsync(new GameMessage_MapRequest(false));
            advanceState(ConnectState::ReceiveMap);
        } else
            onError(ClientError::MapTransmission);
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// server typ
/// @param message  Nachricht, welche ausgeführt wird
bool GameClient::onGameMessage(const GameMessage_GGSChange& msg)
{
    if(state != ClientState::Config && !verifyState(ConnectState::QuerySettings))
        return true;
    LOG.writeToFile("<<< NMS_GGS_CHANGE\n");

    gameLobby->getSettings() = msg.ggs;

    if(state == ClientState::Connect)
    {
        state = ClientState::Config;
        advanceState(ConnectState::Finished);
    } else if(ci)
        ci->CI_GGSChanged(msg.ggs);
    return true;
}

bool GameClient::onGameMessage(const GameMessage_RemoveLua&)
{
    if(state != ClientState::Connect && state != ClientState::Config)
        return true;
    mapinfo.luaFilepath.clear();
    mapinfo.luaData.clear();
    mapinfo.luaChecksum = 0;
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// NFC Antwort vom Server
/// @param message  Nachricht, welche ausgeführt wird
bool GameClient::onGameMessage(const GameMessage_GameCommand& msg)
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

void GameClient::increaseSpeed(const bool wraparound)
{
    const auto curSpeed = framesinfo.gfLengthReq;
    // Note: Higher speed = lower gf_length value
    // Go from debug speed directly back to min speed, else in fixed steps
    static_assert(MIN_SPEED_DEBUG > MIN_SPEED);
    if(framesinfo.gfLengthReq > MIN_SPEED)
        setNewSpeed(MIN_SPEED); // NOLINT(bugprone-branch-clone)
    else
        setNewSpeed((framesinfo.gfLengthReq - 1ms) / SPEED_STEP * SPEED_STEP);
    // If unchanged we capped at max speed, so wrap around if requested
    if(wraparound && framesinfo.gfLengthReq == curSpeed)
        setNewSpeed(MIN_SPEED);
}

void GameClient::decreaseSpeed()
{
    if((DEBUG_MODE || replayMode) && framesinfo.gfLengthReq >= MIN_SPEED)
        setNewSpeed(MIN_SPEED_DEBUG);
    else
        setNewSpeed(framesinfo.gfLengthReq / SPEED_STEP * SPEED_STEP + SPEED_STEP);
}

void GameClient::setNewSpeed(std::chrono::milliseconds gfLength)
{
    static_assert(MIN_SPEED >= SPEED_GF_LENGTHS[GameSpeed::VeryFast], "Not all speeds reachable");
    static_assert(MAX_SPEED <= SPEED_GF_LENGTHS[GameSpeed::VerySlow], "Not all speeds reachable");
    const auto minSpeed = (replayMode || DEBUG_MODE) ? MIN_SPEED_DEBUG : MIN_SPEED;
    const auto maxSpeed = (replayMode || DEBUG_MODE) ? MAX_SPEED_DEBUG : MAX_SPEED;
    const auto oldSpeed = framesinfo.gfLengthReq;
    framesinfo.gfLengthReq = helpers::clamp<decltype(gfLength)>(gfLength, maxSpeed, minSpeed);
    if(replayMode)
        framesinfo.gf_length = framesinfo.gfLengthReq;
    else if(framesinfo.gfLengthReq != oldSpeed)
        mainPlayer.sendMsgAsync(new GameMessage_Speed(framesinfo.gfLengthReq));
}

///////////////////////////////////////////////////////////////////////////////
/// NFC Done vom Server
/// @param message  Nachricht, welche ausgeführt wird
bool GameClient::onGameMessage(const GameMessage_Server_NWFDone& msg)
{
    if(!nwfInfo)
        return true;

    if(!nwfInfo->addServerInfo(NWFServerInfo(msg.gf, std::chrono::milliseconds(msg.gf_length), msg.nextNWF)))
    {
        RTTR_Assert(false);
        LOG.write("Failed to add server info. Invalid server?\n");
    }

    return true;
}

bool GameClient::onGameMessage(const GameMessage_Pause& msg)
{
    // Ignore recorded pause messages in replay mode
    if(state != ClientState::Game || replayMode)
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
bool GameClient::onGameMessage(const GameMessage_GetAsyncLog& /*msg*/)
{
    if(state != ClientState::Game)
        return true;
    std::string systemInfo = System::getCompilerName() + " @ " + System::getOSName();
    mainPlayer.sendMsgAsync(new GameMessage_AsyncLog(systemInfo));

    // AsyncLog an den Server senden

    std::vector<RandomEntry> async_log = RANDOM.getAsyncLog();

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
void GameClient::executeGameFrame()
{
    if(framesinfo.isPaused)
        return; // Pause

    FramesInfo::UsedClock::time_point currentTime = FramesInfo::UsedClock::now();

    if(framesinfo.forcePauseLen.count())
    {
        if(currentTime - framesinfo.forcePauseStart > framesinfo.forcePauseLen)
            framesinfo.forcePauseLen = framesinfo.forcePauseLen.zero();
        else
            return; // Pause
    }

    const unsigned curGF = getGFNumber();
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
                executeGameFrameReplay();
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

                    executeNWF();

                    const auto oldGFLen = framesinfo.gf_length;
                    nwfInfo->execute(framesinfo);
                    if(oldGFLen != framesinfo.gf_length)
                    {
                        LOG.write("Client: Speed changed at %1% from %2% to %3% (NWF: %4%)\n") % curGF
                          % helpers::withUnit(oldGFLen) % helpers::withUnit(framesinfo.gf_length)
                          % framesinfo.nwf_length;
                    }
                }

                nextGF(isNWF);
                RTTR_Assert(curGF <= nwfInfo->getNextNWF());
                handleAutosave();

                // GF-Ende im Replay aktualisieren
                if(replayinfo && replayinfo->replay.isRecording())
                    replayinfo->replay.updateLastGF(curGF);
            }

        } catch(const LuaExecutionError& e)
        {
            systemChat((boost::format(_("Error during execution of lua script: %1%\nGame stopped!")) % e.what()).str());
            onError(ClientError::InvalidMap);
        }
        if(skiptogf == getGFNumber())
            skiptogf = 0;
    }
    framesinfo.frameTime =
      std::chrono::duration_cast<decltype(framesinfo.frameTime)>(currentTime - framesinfo.lastTime);
    // Check remaining time until next GF
    if(framesinfo.frameTime >= framesinfo.gf_length)
    {
        // This can happen, if we don't call this method in intervals less than gf_length or gf_length has changed
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

void GameClient::handleAutosave()
{
    // If inactive or during replay -> no autosave
    if(!SETTINGS.interface.autosaveInterval || replayMode)
        return;

    // Alle .... GF
    if(getGFNumber() % SETTINGS.interface.autosaveInterval == 0)
    {
        std::string filename;
        if(mapinfo.title.empty())
            filename = std::string(_("Auto-Save")) + ".sav";
        else
            filename = mapinfo.title + " (" + _("Auto-Save") + ").sav";

        saveToFile(RTTRCONFIG.expandPath(s25::folders::save) / filename);
    }
}

/// Führt notwendige Dinge für nächsten GF aus
void GameClient::nextGF(bool wasNWF)
{
    for(AIPlayer& ai : game->aiPlayers_)
        ai.runGF(getGFNumber(), wasNWF);
    game->runGF();
}

void GameClient::executeAllGCs(uint8_t playerId, const PlayerGameCommands& gcs)
{
    for(const gc::GameCommandPtr& gc : gcs.gcs)
        gc->execute(game->world_, playerId);
}

void GameClient::sendNothingNC(uint8_t player)
{
    mainPlayer.sendMsgAsync(
      new GameMessage_GameCommand(player, AsyncChecksum::create(*game), std::vector<gc::GameCommandPtr>()));
}

void GameClient::writePlayerInfo(SavedFile& file)
{
    RTTR_Assert(state == ClientState::Loading || state == ClientState::Loaded || state == ClientState::Game);
    // Spielerdaten
    for(unsigned i = 0; i < getNumPlayers(); ++i)
        file.addPlayer(getPlayer(i));
}

void GameClient::onGameStart()
{
    if(state == ClientState::Loaded)
    {
        GAMEMANAGER.resetAverageGFPS();
        framesinfo.lastTime = FramesInfo::UsedClock::now();
        state = ClientState::Game;
        if(ci)
            ci->CI_GameStarted();
    } else if(state == ClientState::Game && !game->isStarted())
    {
        framesinfo.isPaused = replayMode;
        game->start(!!mapinfo.savegame);
    }
}

void GameClient::startReplayRecording(const unsigned random_init)
{
    replayinfo = std::make_unique<ReplayInfo>();
    replayinfo->filename = s25util::Time::FormatTime("%Y-%m-%d_%H-%i-%s") + ".rpl";

    writePlayerInfo(replayinfo->replay);
    replayinfo->replay.ggs = game->ggs_;

    if(!replayinfo->replay.startRecording(RTTRCONFIG.expandPath(s25::folders::replays) / replayinfo->filename, mapinfo,
                                          random_init))
    {
        LOG.write(_("Replayfile couldn't be opened. No replay will be recorded\n"));
        replayinfo.reset();
    }
}

bool GameClient::startReplay(const boost::filesystem::path& path)
{
    RTTR_Assert(state == ClientState::Stopped);
    mapinfo.clear();
    replayinfo = std::make_unique<ReplayInfo>();

    if(!replayinfo->replay.loadHeader(path) || !replayinfo->replay.loadGameData(mapinfo)) //-V807
    {
        LOG.write(_("Invalid Replay %1%! Reason: %2%\n")) % path
          % (replayinfo->replay.getLastErrorMsg().empty() ? _("Unknown") : replayinfo->replay.getLastErrorMsg());
        onError(ClientError::InvalidMap);
        replayinfo.reset();
        return false;
    }
    replayinfo->filename = path.filename();

    gameLobby = std::make_shared<GameLobby>(true, true, replayinfo->replay.getNumPlayers());

    for(unsigned i = 0; i < replayinfo->replay.getNumPlayers(); ++i)
        gameLobby->getPlayer(i) = JoinPlayerInfo(replayinfo->replay.getPlayer(i));

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
            bfs::path mapFilePath = RTTRCONFIG.expandPath(s25::folders::mapsPlayed) / mapinfo.filepath.filename();
            mapinfo.filepath = mapFilePath;
            if(!mapinfo.mapData.decompressToFile(mapinfo.filepath))
            {
                LOG.write(_("Error decompressing map file"));
                onError(ClientError::MapTransmission);
                return false;
            }
            if(mapinfo.luaData.uncompressedLength)
            {
                mapinfo.luaFilepath = mapFilePath.replace_extension("lua");
                if(!mapinfo.luaData.decompressToFile(mapinfo.luaFilepath))
                {
                    LOG.write(_("Error decompressing lua file"));
                    onError(ClientError::MapTransmission);
                    return false;
                }
            }
        }
        break;
        case MapType::Savegame: break;
    }

    replayMode = true;

    try
    {
        startGame(replayinfo->replay.getSeed());
    } catch(const SerializedGameData::Error& error)
    {
        LOG.write(_("Error when loading game from replay: %s\n")) % error.what();
        onError(ClientError::InvalidMap);
        return false;
    }

    replayinfo->nextGF = replayinfo->replay.readGF();

    return true;
}

void GameClient::setAIBattlePlayers(std::vector<AI::Info> aiInfos)
{
    aiBattlePlayers_ = std::move(aiInfos);
}

unsigned GameClient::getGlobalAnimation(const unsigned short max, const unsigned char factor_numerator,
                                        const unsigned char factor_denumerator, const unsigned offset)
{
    // Unit for animations is 630ms (dividable by 2, 3, 5, 6, 7, 10, 15, ...)
    // But this also means: If framerate drops below approx. 15Hz, you won't see
    // every frame of an 8-part animation anymore.
    // An animation runs fully in (factor_numerator / factor_denumerator) multiples of 630ms
    const unsigned unit = 630 /*ms*/ * factor_numerator / factor_denumerator;
    const auto currenttime =
      static_cast<unsigned>((framesinfo.lastTime + framesinfo.frameTime).time_since_epoch() / 1ms);
    return ((currenttime % unit) * max / unit + offset) % max;
}

template<typename T>
T GameClient::doInterpolate(const T x1, const T x2, const GameEvent& ev) const
{
    // TODO: Move to some animation system that is part of game
    std::chrono::milliseconds elapsedTime{};
    if(state == ClientState::Game)
        elapsedTime = (getGFNumber() - ev.startGF) * framesinfo.gf_length + framesinfo.frameTime;
    const auto duration = ev.length * framesinfo.gf_length;
    return helpers::interpolate(x1, x2, elapsedTime, duration);
}

unsigned GameClient::interpolate(const unsigned maxVal, const GameEvent* ev) const
{
    return doInterpolate(0u, maxVal, assertNonNull(ev));
}

int GameClient::interpolate(const int x1, const int x2, const GameEvent* ev) const
{
    return doInterpolate(x1, x2, assertNonNull(ev));
}

void GameClient::serverLost()
{
    onError(ClientError::ConnectionLost);
    // Stop game
    framesinfo.isPaused = true;
}

/**
 *  überspringt eine bestimmte Anzahl von Gameframes.
 *
 *  @param[in] dest_gf Zielgameframe
 */
void GameClient::skipGF(unsigned gf, GameWorldView& gwv)
{
    if(gf <= getGFNumber())
        return;

    unsigned start_ticks = VIDEODRIVER.getTickCount();

    if(!replayMode)
    {
        // unpause before skipping
        setPause(false);
        mainPlayer.sendMsgAsync(new GameMessage_SkipToGF(gf));
        return;
    }

    setPause(false);
    if(getGFNumber() < getLastReplayGF())
        gf = std::min(gf, getLastReplayGF() + 1u);
    skiptogf = gf;

    // GFs überspringen
    for(unsigned i = getGFNumber(); i < skiptogf; ++i)
    {
        if(i % 1000 == 0)
        {
            RoadBuildState road;
            road.mode = RoadBuildMode::Disabled;

            // spiel aktualisieren
            gwv.draw(road, MapPoint::invalid(), false);

            // text oben noch hinschreiben
            boost::format nwfString(_("current GF: %u - still fast forwarding: %d GFs left (%d %%)"));
            nwfString % getGFNumber() % (gf - i) % (i * 100 / gf);
            LargeFont->draw(DrawPoint(VIDEODRIVER.getRenderSize() / 2u), nwfString.str(), FontStyle::CENTER,
                            COLOR_YELLOW);

            VIDEODRIVER.swapBuffers();
        }
        executeGameFrame();
    }
    // Either we force-stopped skipping (skiptogf set to 0) or we reached the target gf
    RTTR_Assert(skiptogf == 0u || getGFNumber() == gf);

    // Spiel pausieren & text ausgabe wie lang das jetzt gedauert hat
    unsigned ticks = VIDEODRIVER.getTickCount() - start_ticks;
    boost::format text(_("Jump finished (%1$.3g seconds)."));
    text % (ticks / 1000.0);
    systemChat(text.str());
    setPause(true);
}

void GameClient::systemChat(const std::string& text)
{
    systemChat(text, getPlayerId());
}

void GameClient::systemChat(const std::string& text, unsigned char fromPlayerIdx)
{
    if(ci)
        ci->CI_Chat(fromPlayerIdx, ChatDestination::System, text);
}

bool GameClient::saveToFile(const boost::filesystem::path& filepath)
{
    mainPlayer.sendMsg(GameMessage_Chat(getPlayerId(), ChatDestination::System, _("Saving game...")));

    // Mond malen
    Position moonPos = VIDEODRIVER.getMousePos();
    moonPos.y -= 40;
    LOADER.getImageN("resource", 33)->drawFull(moonPos);
    VIDEODRIVER.swapBuffers();

    Savegame save;

    writePlayerInfo(save);

    // GGS-Daten
    save.ggs = game->ggs_;

    save.start_gf = getGFNumber();

    // Enable/Disable debugging of savegames
    save.sgd.debugMode = SETTINGS.global.debugMode;

    try
    {
        // Spiel serialisieren
        save.sgd.makeSnapshot(*game);
        // Und alles speichern
        return save.save(filepath, mapinfo.title);
    } catch(const std::exception& e)
    {
        systemChat(std::string(_("Error during saving: ")) + e.what());
        return false;
    }
}

void GameClient::resetVisualSettings()
{
    getPlayer(getPlayerId()).fillVisualSettings(visual_settings);
}

void GameClient::setPause(bool pause)
{
    if(state == ClientState::Stopped)
    {
        // We can never continue from pause if stopped as the reason for stopping might be that the game was finished
        // However we allow to pause even when stopped so we can pause after we received the stop notification
        if(!pause)
            return;
        framesinfo.isPaused = true;
        framesinfo.frameTime = 0ms;
    } else if(replayMode)
    {
        // Pause instantly
        framesinfo.isPaused = pause;
        framesinfo.frameTime = 0ms;
    } else if(isHost())
    {
        auto* msg = new GameMessage_Pause(pause);
        if(pause)
            onGameMessage(*msg);
        mainPlayer.sendMsgAsync(msg);
    }
}

void GameClient::setReplayFOW(const bool hideFOW)
{
    if(replayinfo)
        replayinfo->allVisible = hideFOW;
}

bool GameClient::isReplayFOWDisabled() const
{
    return replayMode && replayinfo->allVisible;
}

unsigned GameClient::getLastReplayGF() const
{
    return replayinfo ? replayinfo->replay.getLastGF() : 0u;
}

bool GameClient::addGC(gc::GameCommandPtr gc)
{
    // Nicht in der Pause oder wenn er besiegt wurde
    if(framesinfo.isPaused || getPlayer(getPlayerId()).isDefeated() || isReplayModeOn())
        return false;

    gameCommands_.push_back(gc);
    return true;
}

unsigned GameClient::getNumPlayers() const
{
    RTTR_Assert(state == ClientState::Loading || state == ClientState::Loaded || state == ClientState::Game);
    return game->world_.getNumPlayers();
}

GamePlayer& GameClient::getPlayer(const unsigned id)
{
    RTTR_Assert(state == ClientState::Loading || state == ClientState::Loaded || state == ClientState::Game);
    RTTR_Assert(id < getNumPlayers());
    return game->world_.getPlayer(id);
}

std::unique_ptr<AIPlayer> GameClient::createAIPlayer(unsigned playerId, const AI::Info& aiInfo)
{
    return AIFactory::create(aiInfo, playerId, game->world_);
}

/// Wandelt eine GF-Angabe in eine Zeitangabe um (HH:MM:SS oder MM:SS wenn Stunden = 0)
std::string GameClient::formatGFTime(const unsigned gf) const
{
    using seconds = std::chrono::duration<uint32_t, std::chrono::seconds::period>;
    using hours = std::chrono::duration<uint32_t, std::chrono::hours::period>;
    using minutes = std::chrono::duration<uint32_t, std::chrono::minutes::period>;
    using std::chrono::duration_cast;

    seconds numSeconds = duration_cast<seconds>(gfsToDuration(gf));

    hours numHours = duration_cast<hours>(numSeconds);
    numSeconds -= numHours;
    minutes numMinutes = duration_cast<minutes>(numSeconds);
    numSeconds -= numMinutes;

    // Use hour format only if we have hours
    if(numHours.count())
        return helpers::format("%u:%02u:%02u", numHours.count(), numMinutes.count(), numSeconds.count());
    else
        return helpers::format("%02u:%02u", numMinutes.count(), numSeconds.count());
}

const boost::filesystem::path& GameClient::getReplayFilename() const
{
    static boost::filesystem::path emptyString;
    return replayinfo ? replayinfo->filename : emptyString;
}

Replay* GameClient::getReplay()
{
    return replayinfo ? &replayinfo->replay : nullptr;
}

std::shared_ptr<const NWFInfo> GameClient::getNWFInfo() const
{
    return nwfInfo;
}

/// Is tournament mode activated (0 if not)? Returns the durations of the tournament mode in gf otherwise
unsigned GameClient::getTournamentModeDuration() const
{
    using namespace std::chrono;
    if(game && rttr::enumCast(game->ggs_.objective) >= rttr::enumCast(GameObjective::Tournament1)
       && static_cast<unsigned>(rttr::enumCast(game->ggs_.objective))
            < rttr::enumCast(GameObjective::Tournament1) + NUM_TOURNAMENT_MODES)
    {
        const auto turnamentMode = rttr::enumCast(game->ggs_.objective) - rttr::enumCast(GameObjective::Tournament1);
        return durationToGfs(TOURNAMENT_MODES_DURATION[turnamentMode]);
    } else
        return 0;
}

void GameClient::toggleHumanAIPlayer(const AI::Info& aiInfo)
{
    RTTR_Assert(!isReplayModeOn());
    auto it = helpers::findIf(game->aiPlayers_,
                              [id = this->getPlayerId()](const auto& player) { return player.getPlayerId() == id; });
    if(it != game->aiPlayers_.end())
    {
        game->aiPlayers_.erase(it);
        systemChat(_("Disabled AI for current player"));
    } else
    {
        game->addAIPlayer(createAIPlayer(getPlayerId(), aiInfo));
        systemChat(_("Enabled AI for current player"));
    }
}

void GameClient::requestSwapToPlayer(const unsigned char newId)
{
    if(state != ClientState::Game)
        return;
    GamePlayer& player = getPlayer(newId);
    if(player.ps == PlayerState::AI && player.aiInfo.type == AI::Type::Dummy)
        mainPlayer.sendMsgAsync(new GameMessage_Player_Swap(0xFF, newId));
}
