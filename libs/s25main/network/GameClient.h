// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "ClientError.h"
#include "FramesInfo.h"
#include "GameCommand.h"
#include "GameMessageInterface.h"
#include "ILocalGameState.h"
#include "NetworkPlayer.h"
#include "factories/GameCommandFactory.h"
#include "gameTypes/AIInfo.h"
#include "gameTypes/ChatDestination.h"
#include "gameTypes/MapDescription.h"
#include "gameTypes/MapInfo.h"
#include "gameTypes/Nation.h"
#include "gameTypes/ServerType.h"
#include "gameTypes/TeamTypes.h"
#include "gameTypes/VisualSettings.h"
#include "s25util/Singleton.h"
#include <memory>
#include <vector>

namespace AI {
struct Info;
}

class AIPlayer;
class ClientInterface;
class Game;
class GameEvent;
class GameLobby;
class GamePlayer;
class GameWorldView;
class NWFInfo;
class Replay;
class SavedFile;
enum class ConnectState;
struct CreateServerInfo;
struct PlayerGameCommands;
struct ReplayInfo;

enum class ClientState
{
    Stopped,
    Connect,
    Config,
    Loading,
    Loaded,
    Game
};

class GameClient final :
    public Singleton<GameClient, SingletonPolicies::WithLongevity>,
    public GameMessageInterface,
    public GameCommandFactory,
    public ILocalGameState
{
public:
    static constexpr unsigned Longevity = 5;

    GameClient();
    ~GameClient();

    void setInterface(ClientInterface* ci) { this->ci = ci; }
    /// Removes the given interface (if it is not yet overwritten by another one)
    void removeInterface(ClientInterface* ci)
    {
        if(this->ci == ci)
            this->ci = nullptr;
    }
    bool isHost() const override { return clientconfig.isHost; }
    /// Manually set the host status. Normally done in connect call
    void setIsHost(bool isHost) { clientconfig.isHost = isHost; }
    std::string getGameName() const { return clientconfig.gameName; }

    unsigned getPlayerId() const override { return mainPlayer.playerId; }

    bool connect(const std::string& server, const std::string& password, ServerType servertyp, unsigned short port,
                 bool host, bool use_ipv6);

    /// Start the server and connect to it
    bool hostGame(const CreateServerInfo& csi, const MapDescription& map);
    void run();
    void stop();

    /// Gibt Map-Titel zurück
    const std::string& getMapTitle() const { return mapinfo.title; }
    /// Gibt Pfad zu der Map zurück
    const boost::filesystem::path& getMapPath() const { return mapinfo.filepath; }
    /// Gibt Map-Typ zurück
    MapType getMapType() const { return mapinfo.type; }
    const boost::filesystem::path& getLuaFilePath() const { return mapinfo.luaFilepath; }

    // Initialisiert und startet das Spiel
    void startGame(unsigned random_init);
    /// Called when the game is loaded
    void gameLoaded();

    /// Beendet das Spiel, zerstört die Spielstrukturen
    void exitGame();

    ClientState getState() const { return state; }
    Replay* getReplay();
    std::shared_ptr<const NWFInfo> getNWFInfo() const;
    std::shared_ptr<GameLobby> getGameLobby();
    const AIPlayer* getAIPlayer(unsigned id) const;

    unsigned getGFNumber() const;
    std::chrono::milliseconds getGFLength() const { return framesinfo.gf_length; }
    unsigned getNWFLength() const { return framesinfo.nwf_length; }
    std::chrono::milliseconds getFrameTime() const { return framesinfo.frameTime; }
    unsigned getGlobalAnimation(unsigned short max, unsigned char factor_numerator, unsigned char factor_denumerator,
                                unsigned offset);
    unsigned interpolate(unsigned maxVal, const GameEvent* ev) const;
    int interpolate(int x1, int x2, const GameEvent* ev) const;

    void commandChat(const std::string& text, ChatDestination cd);
    void commandSetNation(Nation newNation);
    void commandSetPortrait(unsigned portraitIndex);
    void commandSetTeam(Team newTeam);
    void commandSetColor(unsigned newColor);
    void commandSetReady(bool isReady);

    /// Called internally when the game is ready to start (loaded and all players ready)
    /// And a 2nd time when the GUI is ready which actually starty the game
    void onGameStart();

    void increaseSpeed(bool wraparound = false);
    void decreaseSpeed();
    void setNewSpeed(std::chrono::milliseconds gfLength);
    // Used by tests (stinks, but what to do?)
    std::chrono::milliseconds getGFLengthReq() { return framesinfo.gfLengthReq; }

    /// Lädt ein Replay und startet dementsprechend das Spiel
    bool startReplay(const boost::filesystem::path& path);

    /// When a non-empty vector is given then an AI battle with the given AIs is started
    void setAIBattlePlayers(std::vector<AI::Info> aiInfos);
    const std::vector<AI::Info>& getAIBattlePlayers() const { return aiBattlePlayers_; }
    bool isAIBattleModeOn() const { return !aiBattlePlayers_.empty(); }

    void setPause(bool pause);
    void togglePause() { setPause(!framesinfo.isPaused); }
    /// Hide or show the fog-of-war. Only in replay mode
    void setReplayFOW(bool hideFOW);
    /// Return whether we are in replay mode and fog-of-war is disabled
    bool isReplayFOWDisabled() const;
    /// Gibt Replay-Ende (GF) zurück
    unsigned getLastReplayGF() const;
    /// Wandelt eine GF-Angabe in eine Zeitangabe um (HH:MM:SS oder MM:SS wenn Stunden = 0)
    std::string formatGFTime(unsigned gf) const override;

    /// Gibt Replay-Dateiname zurück
    const boost::filesystem::path& getReplayFilename() const;
    /// Wird ein Replay abgespielt?
    bool isReplayModeOn() const { return replayMode; }

    /// Is tournament mode activated (0 if not)? Returns the durations of the tournament mode in gf otherwise
    unsigned getTournamentModeDuration() const;

    void skipGF(unsigned gf, GameWorldView& gwv);

    /// Changes the player ingame (for replay or debugging)
    void changePlayerIngame(unsigned char playerId1, unsigned char playerId2);
    /// Sends a request to swap places with the requested player. Only for debugging!
    void requestSwapToPlayer(unsigned char newId);

    /// Spiel pausiert?
    bool isPaused() const { return framesinfo.isPaused; }
    /// Schreibt Header der Save-Datei
    bool saveToFile(const boost::filesystem::path& filepath);
    /// Visuelle Einstellungen aus den richtigen ableiten
    void resetVisualSettings();
    void systemChat(const std::string& text) override;
    void systemChat(const std::string& text, unsigned char fromPlayerIdx);

    /// Toggle current player to be an AI player of the given type
    void toggleHumanAIPlayer(const AI::Info& aiInfo);

    NetworkPlayer& getMainPlayer() { return mainPlayer; }

private:
    /// Create an AI player for the current world
    std::unique_ptr<AIPlayer> createAIPlayer(unsigned playerId, const AI::Info& aiInfo);

    /// Add the gamecommand. Return true in success, false otherwise (paused, or defeated)
    bool addGC(gc::GameCommandPtr gc) override;

    unsigned getNumPlayers() const;
    /// Liefert einen Player zurück
    GamePlayer& getPlayer(unsigned id);

    /// Versucht einen neuen GameFrame auszuführen, falls die Zeit dafür gekommen ist
    void executeGameFrame();
    void executeGameFrameReplay();
    void executeNWF();
    /// Filtert aus einem Network-Command-Paket alle Commands aus und führt sie aus, falls ein Spielerwechsel-Command
    /// dabei ist, füllt er die übergebenen IDs entsprechend aus
    void executeAllGCs(uint8_t playerId, const PlayerGameCommands& gcs);
    /// Sendet ein NC-Paket ohne Befehle
    void sendNothingNC(uint8_t player = 0xFF);

    /// Führt notwendige Dinge für nächsten GF aus
    void nextGF(bool wasNWF);
    /// Checks if its time for autosaving (if enabled) and does it
    void handleAutosave();

    /// Interpolate implementation for generic types
    /// Returns x in [x1, x2] according to the current time in the range of the events duration.
    template<typename T>
    T doInterpolate(T x1, T x2, const GameEvent& ev) const;

    //  Netzwerknachrichten
    RTTR_IGNORE_OVERLOADED_VIRTUAL
    bool onGameMessage(const GameMessage_Ping& msg) override;

    bool onGameMessage(const GameMessage_Server_TypeOK& msg) override;
    bool onGameMessage(const GameMessage_Server_Password& msg) override;
    bool onGameMessage(const GameMessage_Server_Name& msg) override;
    bool onGameMessage(const GameMessage_Server_Start& msg) override;
    bool onGameMessage(const GameMessage_Chat& msg) override;
    bool onGameMessage(const GameMessage_Server_Async& msg) override;
    bool onGameMessage(const GameMessage_Countdown& msg) override;
    bool onGameMessage(const GameMessage_CancelCountdown& msg) override;

    bool onGameMessage(const GameMessage_Player_Id& msg) override;
    bool onGameMessage(const GameMessage_Player_List& msg) override;
    bool onGameMessage(const GameMessage_Player_Name& msg) override;
    bool onGameMessage(const GameMessage_Player_Portrait& msg) override;
    bool onGameMessage(const GameMessage_Player_State& msg) override;
    bool onGameMessage(const GameMessage_Player_Nation& msg) override;
    bool onGameMessage(const GameMessage_Player_Team& msg) override;
    bool onGameMessage(const GameMessage_Player_Color& msg) override;
    bool onGameMessage(const GameMessage_Player_Kicked& msg) override;
    bool onGameMessage(const GameMessage_Player_Ping& msg) override;
    bool onGameMessage(const GameMessage_Player_New& msg) override;
    bool onGameMessage(const GameMessage_Player_Ready& msg) override;
    bool onGameMessage(const GameMessage_Player_Swap& msg) override;

    bool onGameMessage(const GameMessage_Map_Info& msg) override;
    bool onGameMessage(const GameMessage_Map_Data& msg) override;
    bool onGameMessage(const GameMessage_Map_ChecksumOK& msg) override;

    bool onGameMessage(const GameMessage_Pause& msg) override;
    bool onGameMessage(const GameMessage_SkipToGF& msg) override;
    bool onGameMessage(const GameMessage_Server_NWFDone& msg) override;
    bool onGameMessage(const GameMessage_GameCommand& msg) override;

    bool onGameMessage(const GameMessage_GGSChange& msg) override;
    bool onGameMessage(const GameMessage_RemoveLua& msg) override;

    bool onGameMessage(const GameMessage_GetAsyncLog& msg) override;
    RTTR_POP_DIAGNOSTIC

    /// Report the error and stop
    void onError(ClientError error);
    /// Advance to new connect state
    void advanceState(ConnectState newState);
    /// Verifies that the current connect state matches the expected one
    /// On error the error is reported and the connection terminated as likely the server is faulty
    bool verifyState(ConnectState expectedState);

    bool createLobby();

    /// Wird aufgerufen, wenn der Server gegangen ist (Verbindung verloren, ungültige Nachricht etc.)
    void serverLost();

    // Replaymethoden

    /// Schreibt den Header der Replaydatei
    void startReplayRecording(unsigned random_init);
    void writePlayerInfo(SavedFile& file);

public:
    /// Virtuelle Werte der Einstellungsfenster, die aber noch nicht wirksam sind, nur um die Verzögerungen zu
    /// verstecken
    // TODO: Move to viewer
    VisualSettings visual_settings, default_settings; //-V730_NOINIT
    /// skip ahead how many gf?
    unsigned skiptogf;

private:
    NetworkPlayer mainPlayer;

    ClientState state;
    ConnectState connectState;

    /// Game state itself (valid during LOADING and GAME state)
    std::shared_ptr<Game> game;
    /// NWF info
    std::shared_ptr<NWFInfo> nwfInfo;
    /// Game lobby (valid during CONFIG state)
    std::shared_ptr<GameLobby> gameLobby;

    class ClientConfig
    {
    public:
        ClientConfig() { clear(); }
        void clear();

        std::string server;
        std::string gameName;
        std::string password;
        ServerType servertyp;
        unsigned short port;
        bool isHost;
    } clientconfig;

    MapInfo mapinfo;

    FramesInfoClient framesinfo;

    ClientInterface* ci;

    /// GameCommands, die vom Client noch an den Server gesendet werden müssen
    std::vector<gc::GameCommandPtr> gameCommands_;

    std::unique_ptr<ReplayInfo> replayinfo;
    bool replayMode;

    /// Configured players for an AI battle.
    std::vector<AI::Info> aiBattlePlayers_;
};

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#define GAMECLIENT GameClient::inst()
