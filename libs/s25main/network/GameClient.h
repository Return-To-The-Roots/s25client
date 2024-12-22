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

    void SetInterface(ClientInterface* ci) { this->ci = ci; }
    /// Removes the given interface (if it is not yet overwritten by another one)
    void RemoveInterface(ClientInterface* ci)
    {
        if(this->ci == ci)
            this->ci = nullptr;
    }
    bool IsHost() const override { return clientconfig.isHost; }
    /// Manually set the host status. Normally done in connect call
    void SetIsHost(bool isHost) { clientconfig.isHost = isHost; }
    std::string GetGameName() const { return clientconfig.gameName; }

    unsigned GetPlayerId() const override { return mainPlayer.playerId; }

    bool Connect(const std::string& server, const std::string& password, ServerType servertyp, unsigned short port,
                 bool host, bool use_ipv6);

    /// Start the server and connect to it
    bool HostGame(const CreateServerInfo& csi, const MapDescription& map);
    void Run();
    void Stop();

    /// Gibt Map-Titel zurück
    const std::string& GetMapTitle() const { return mapinfo.title; }
    /// Gibt Pfad zu der Map zurück
    const boost::filesystem::path& GetMapPath() const { return mapinfo.filepath; }
    /// Gibt Map-Typ zurück
    MapType GetMapType() const { return mapinfo.type; }
    const boost::filesystem::path& GetLuaFilePath() const { return mapinfo.luaFilepath; }

    // Initialisiert und startet das Spiel
    void StartGame(unsigned random_init);
    /// Called when the game is loaded
    void GameLoaded();

    /// Beendet das Spiel, zerstört die Spielstrukturen
    void ExitGame();

    void SetCampaignChapterCompleted(unsigned char chapter) { chapterCompleted = chapter; }
    void SetCampaignCompleted(bool state) { campaignCompleted = state; }
    unsigned char GetCampaignChapterCompleted() const { return chapterCompleted; }
    bool IsCampaignCompleted() const { return campaignCompleted; }

    ClientState GetState() const { return state; }
    Replay* GetReplay();
    std::shared_ptr<const NWFInfo> GetNWFInfo() const;
    std::shared_ptr<GameLobby> GetGameLobby();
    const AIPlayer* GetAIPlayer(unsigned id) const;

    unsigned GetGFNumber() const;
    FramesInfo::milliseconds32_t GetGFLength() const { return framesinfo.gf_length; }
    unsigned GetNWFLength() const { return framesinfo.nwf_length; }
    FramesInfo::milliseconds32_t GetFrameTime() const { return framesinfo.frameTime; }
    unsigned GetGlobalAnimation(unsigned short max, unsigned char factor_numerator, unsigned char factor_denumerator,
                                unsigned offset);
    unsigned Interpolate(unsigned max_val, const GameEvent* ev);
    int Interpolate(int x1, int x2, const GameEvent* ev);

    void Command_Chat(const std::string& text, ChatDestination cd);
    void Command_SetNation(Nation newNation);
    void Command_SetTeam(Team newTeam);
    void Command_SetColor(unsigned newColor);
    void Command_SetReady(bool isReady);

    /// Called internally when the game is ready to start (loaded and all players ready)
    /// And a 2nd time when the GUI is ready which actually starty the game
    void OnGameStart();

    void IncreaseSpeed();
    void DecreaseSpeed();

    /// Lädt ein Replay und startet dementsprechend das Spiel
    bool StartReplay(const boost::filesystem::path& path);

    /// When a non-empty vector is given then an AI battle with the given AIs is started
    void SetAIBattlePlayers(std::vector<AI::Info> aiInfos);
    const std::vector<AI::Info>& GetAIBattlePlayers() const { return aiBattlePlayers_; }
    bool IsAIBattleModeOn() const { return !aiBattlePlayers_.empty(); }

    void SetPause(bool pause);
    void TogglePause() { SetPause(!framesinfo.isPaused); }
    /// Hide or show the fog-of-war. Only in replay mode
    void SetReplayFOW(bool hideFOW);
    /// Return whether we are in replay mode and fog-of-war is disabled
    bool IsReplayFOWDisabled() const;
    /// Gibt Replay-Ende (GF) zurück
    unsigned GetLastReplayGF() const;
    /// Wandelt eine GF-Angabe in eine Zeitangabe um (HH:MM:SS oder MM:SS wenn Stunden = 0)
    std::string FormatGFTime(unsigned gf) const override;

    /// Gibt Replay-Dateiname zurück
    const boost::filesystem::path& GetReplayFilename() const;
    /// Wird ein Replay abgespielt?
    bool IsReplayModeOn() const { return replayMode; }

    /// Is tournament mode activated (0 if not)? Returns the durations of the tournament mode in gf otherwise
    unsigned GetTournamentModeDuration() const;

    void SkipGF(unsigned gf, GameWorldView& gwv);

    /// Changes the player ingame (for replay or debugging)
    void ChangePlayerIngame(unsigned char playerId1, unsigned char playerId2);
    /// Sends a request to swap places with the requested player. Only for debugging!
    void RequestSwapToPlayer(unsigned char newId);

    /// Spiel pausiert?
    bool IsPaused() const { return framesinfo.isPaused; }
    /// Schreibt Header der Save-Datei
    bool SaveToFile(const boost::filesystem::path& filepath);
    /// Visuelle Einstellungen aus den richtigen ableiten
    void ResetVisualSettings();
    void SystemChat(const std::string& text) override;
    void SystemChat(const std::string& text, unsigned char fromPlayerIdx);

    /// Toggle current player to be an AI player of the given type
    void ToggleHumanAIPlayer(const AI::Info& aiInfo);

    NetworkPlayer& GetMainPlayer() { return mainPlayer; }

private:
    /// Create an AI player for the current world
    std::unique_ptr<AIPlayer> CreateAIPlayer(unsigned playerId, const AI::Info& aiInfo);

    /// Add the gamecommand. Return true in success, false otherwise (paused, or defeated)
    bool AddGC(gc::GameCommandPtr gc) override;

    unsigned GetNumPlayers() const;
    /// Liefert einen Player zurück
    GamePlayer& GetPlayer(unsigned id);

    /// Versucht einen neuen GameFrame auszuführen, falls die Zeit dafür gekommen ist
    void ExecuteGameFrame();
    void ExecuteGameFrame_Replay();
    void ExecuteNWF();
    /// Filtert aus einem Network-Command-Paket alle Commands aus und führt sie aus, falls ein Spielerwechsel-Command
    /// dabei ist, füllt er die übergebenen IDs entsprechend aus
    void ExecuteAllGCs(uint8_t playerId, const PlayerGameCommands& gcs);
    /// Sendet ein NC-Paket ohne Befehle
    void SendNothingNC(uint8_t player = 0xFF);

    /// Führt notwendige Dinge für nächsten GF aus
    void NextGF(bool wasNWF);
    /// Checks if its time for autosaving (if enabled) and does it
    void HandleAutosave();

    //  Netzwerknachrichten
    RTTR_IGNORE_OVERLOADED_VIRTUAL
    bool OnGameMessage(const GameMessage_Ping& msg) override;

    bool OnGameMessage(const GameMessage_Server_TypeOK& msg) override;
    bool OnGameMessage(const GameMessage_Server_Password& msg) override;
    bool OnGameMessage(const GameMessage_Server_Name& msg) override;
    bool OnGameMessage(const GameMessage_Server_Start& msg) override;
    bool OnGameMessage(const GameMessage_Chat& msg) override;
    bool OnGameMessage(const GameMessage_Server_Async& msg) override;
    bool OnGameMessage(const GameMessage_Countdown& msg) override;
    bool OnGameMessage(const GameMessage_CancelCountdown& msg) override;

    bool OnGameMessage(const GameMessage_Player_Id& msg) override;
    bool OnGameMessage(const GameMessage_Player_List& msg) override;
    bool OnGameMessage(const GameMessage_Player_Name& msg) override;
    bool OnGameMessage(const GameMessage_Player_State& msg) override;
    bool OnGameMessage(const GameMessage_Player_Nation& msg) override;
    bool OnGameMessage(const GameMessage_Player_Team& msg) override;
    bool OnGameMessage(const GameMessage_Player_Color& msg) override;
    bool OnGameMessage(const GameMessage_Player_Kicked& msg) override;
    bool OnGameMessage(const GameMessage_Player_Ping& msg) override;
    bool OnGameMessage(const GameMessage_Player_New& msg) override;
    bool OnGameMessage(const GameMessage_Player_Ready& msg) override;
    bool OnGameMessage(const GameMessage_Player_Swap& msg) override;

    bool OnGameMessage(const GameMessage_Map_Info& msg) override;
    bool OnGameMessage(const GameMessage_Map_Data& msg) override;
    bool OnGameMessage(const GameMessage_Map_ChecksumOK& msg) override;

    bool OnGameMessage(const GameMessage_Pause& msg) override;
    bool OnGameMessage(const GameMessage_SkipToGF& msg) override;
    bool OnGameMessage(const GameMessage_Server_NWFDone& msg) override;
    bool OnGameMessage(const GameMessage_GameCommand& msg) override;

    bool OnGameMessage(const GameMessage_GGSChange& msg) override;
    bool OnGameMessage(const GameMessage_RemoveLua& msg) override;

    bool OnGameMessage(const GameMessage_GetAsyncLog& msg) override;
    RTTR_POP_DIAGNOSTIC

    /// Report the error and stop
    void OnError(ClientError error);
    /// Advance to new connect state
    void AdvanceState(ConnectState newState);
    /// Verifies that the current connect state matches the expected one
    /// On error the error is reported and the connection terminated as likely the server is faulty
    bool VerifyState(ConnectState expectedState);

    bool CreateLobby();

    /// Wird aufgerufen, wenn der Server gegangen ist (Verbindung verloren, ungültige Nachricht etc.)
    void ServerLost();

    // Replaymethoden

    /// Schreibt den Header der Replaydatei
    void StartReplayRecording(unsigned random_init);
    void WritePlayerInfo(SavedFile& file);

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
        ClientConfig() { Clear(); }
        void Clear();

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

    unsigned char chapterCompleted = 0;
    bool campaignCompleted = false;

    std::unique_ptr<ReplayInfo> replayinfo;
    bool replayMode;

    /// Configured players for an AI battle.
    std::vector<AI::Info> aiBattlePlayers_;
};

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#define GAMECLIENT GameClient::inst()
