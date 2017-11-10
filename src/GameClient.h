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
#ifndef GAMECLIENT_H_
#define GAMECLIENT_H_

#include "FramesInfo.h"
#include "GameCommand.h"
#include "GameMessageInterface.h"
#include "factories/GameCommandFactory.h"
#include "helpers/Deleter.h"
#include "gameTypes/ChatDestination.h"
#include "gameTypes/MapInfo.h"
#include "gameTypes/ServerType.h"
#include "gameTypes/TeamTypes.h"
#include "gameTypes/VisualSettings.h"
#include "libutil/MessageQueue.h"
#include "libutil/Singleton.h"
#include "libutil/Socket.h"
#include <boost/interprocess/smart_ptr/unique_ptr.hpp>
#include <boost/shared_ptr.hpp>

namespace AI {
struct Info;
}

class AIPlayer;
class ClientInterface;
class SavedFile;
class GamePlayer;
class GameEvent;
class GameLobby;
class GameWorldView;
class Game;
class Replay;
class EventManager;
struct PlayerGameCommands;
struct ReplayInfo;

class GameClient : public Singleton<GameClient, SingletonPolicies::WithLongevity>, public GameMessageInterface, public GameCommandFactory
{
public:
    BOOST_STATIC_CONSTEXPR unsigned Longevity = 5;

    enum ClientState
    {
        CS_STOPPED = 0,
        CS_CONNECT,
        CS_CONFIG,
        CS_LOADING,
        CS_GAME
    };

    GameClient();
    ~GameClient() override;

    void SetInterface(ClientInterface* ci) { this->ci = ci; }
    /// Removes the given interface (if it is not yet overwritten by another one)
    void RemoveInterface(ClientInterface* ci)
    {
        if(this->ci == ci)
            this->ci = NULL;
    }
    bool IsHost() const { return clientconfig.isHost; }
    std::string GetGameName() const { return clientconfig.gameName; }

    unsigned GetPlayerId() const { return playerId_; }
    /// Erzeugt einen KI-Player, der mit den Daten vom GameClient gefüttert werden muss
    AIPlayer* CreateAIPlayer(unsigned playerId, const AI::Info& aiInfo);

    bool Connect(const std::string& server, const std::string& password, ServerType servertyp, unsigned short port, bool host,
                 bool use_ipv6);
    void Run();
    void Stop();

    /// Gibt Map-Titel zurück
    const std::string& GetMapTitle() const { return mapinfo.title; }
    /// Gibt Pfad zu der Map zurück
    const std::string& GetMapPath() const { return mapinfo.filepath; }
    /// Gibt Map-Typ zurück
    const MapType GetMapType() const { return mapinfo.type; }
    const std::string& GetLuaFilePath() const { return mapinfo.luaFilepath; }
    GameLobby& GetGameLobby();

    // Initialisiert und startet das Spiel
    void StartGame(const unsigned random_init);
    /// Wird aufgerufen, wenn das GUI fertig mit Laden ist und es losgehen kann
    void GameStarted();

    /// Beendet das Spiel, zerstört die Spielstrukturen
    void ExitGame();

    ClientState GetState() const { return state; }
    unsigned GetGFNumber() const;
    unsigned GetGFLength() const { return framesinfo.gf_length; }
    unsigned GetNWFLength() const { return framesinfo.nwf_length; }
    unsigned GetFrameTime() const { return framesinfo.frameTime; }
    unsigned GetGlobalAnimation(const unsigned short max, const unsigned char factor_numerator, const unsigned char factor_denumerator,
                                const unsigned offset);
    unsigned Interpolate(unsigned max_val, const GameEvent* ev);
    int Interpolate(int x1, int x2, const GameEvent* ev);

    void Command_Chat(const std::string& text, const ChatDestination cd);
    void Command_SetNation(Nation newNation);
    void Command_SetTeam(Team newTeam);
    void Command_SetColor(unsigned newColor);
    void Command_SetReady(bool isReady);

    void IncreaseSpeed();
    void DecreaseSpeed();

    /// Lädt ein Replay und startet dementsprechend das Spiel
    bool StartReplay(const std::string& path);
    /// Replay-Geschwindigkeit erhöhen/verringern
    void IncreaseReplaySpeed();
    void DecreaseReplaySpeed();
    void SetReplayPause(bool pause);
    void ToggleReplayPause() { SetReplayPause(!framesinfo.isPaused); }
    /// Schaltet FoW im Replaymodus ein/aus
    void ToggleReplayFOW();
    /// Prüft, ob FoW im Replaymodus ausgeschalten ist
    bool IsReplayFOWDisabled() const;
    /// Gibt Replay-Ende (GF) zurück
    unsigned GetLastReplayGF() const;
    /// Wandelt eine GF-Angabe in eine Zeitangabe um (HH:MM:SS oder MM:SS wenn Stunden = 0)
    std::string FormatGFTime(const unsigned gf) const;

    /// Gibt Replay-Dateiname zurück
    const std::string& GetReplayFileName() const;
    /// Wird ein Replay abgespielt?
    bool IsReplayModeOn() const { return replayMode; }

    Replay& GetReplay();

    /// Is tournament mode activated (0 if not)? Returns the durations of the tournament mode in gf otherwise
    unsigned GetTournamentModeDuration() const;

    void SkipGF(unsigned gf, GameWorldView& gwv);

    /// Changes the player ingame (for replay or debugging)
    void ChangePlayerIngame(const unsigned char player1, const unsigned char player2);
    /// Sends a request to swap places with the requested player. Only for debugging!
    void RequestSwapToPlayer(const unsigned char newId);

    /// Laggt ein bestimmter Spieler gerade?
    bool IsLagging(const unsigned id);
    /// Spiel pausiert?
    bool IsPaused() const { return framesinfo.isPaused; }
    /// Schreibt Header der Save-Datei
    bool SaveToFile(const std::string& filename);
    /// Visuelle Einstellungen aus den richtigen ableiten
    void ResetVisualSettings();
    void SystemChat(const std::string& text, unsigned char player = 0xFF);

    void ToggleHumanAIPlayer();

private:
    /// Fügt ein GameCommand für den Spieler hinzu und gibt bei Erfolg true zurück, ansonstn false (in der Pause oder wenn Spieler besiegt
    /// ist)
    bool AddGC(gc::GameCommand* gc) override;

    unsigned GetPlayerCount() const;
    /// Liefert einen Player zurück
    GamePlayer& GetPlayer(const unsigned id);

    /// Versucht einen neuen GameFrame auszuführen, falls die Zeit dafür gekommen ist
    void ExecuteGameFrame(const bool skipping = false);
    void ExecuteGameFrame_Replay();
    void ExecuteNWF();
    /// Filtert aus einem Network-Command-Paket alle Commands aus und führt sie aus, falls ein Spielerwechsel-Command
    /// dabei ist, füllt er die übergebenen IDs entsprechend aus
    void ExecuteAllGCs(uint8_t playerId, const PlayerGameCommands& gcs);
    /// Sendet ein NC-Paket ohne Befehle
    void SendNothingNC();
    /// Findet heraus, ob ein Spieler laggt und setzt bei diesen Spieler den entsprechenden flag
    bool IsPlayerLagging();

    /// Führt notwendige Dinge für nächsten GF aus
    void NextGF();
    /// Checks if its time for autosaving (if enabled) and does it
    void HandleAutosave();

    //  Netzwerknachrichten
    bool OnGameMessage(const GameMessage_Ping& msg) override;

    bool OnGameMessage(const GameMessage_Server_TypeOK& msg) override;
    bool OnGameMessage(const GameMessage_Server_Password& msg) override;
    bool OnGameMessage(const GameMessage_Server_Name& msg) override;
    bool OnGameMessage(const GameMessage_Server_Start& msg) override;
    bool OnGameMessage(const GameMessage_Server_Chat& msg) override;
    bool OnGameMessage(const GameMessage_System_Chat& msg) override;
    bool OnGameMessage(const GameMessage_Server_Async& msg) override;
    bool OnGameMessage(const GameMessage_Server_Countdown& msg) override;
    bool OnGameMessage(const GameMessage_Server_CancelCountdown& msg) override;

    bool OnGameMessage(const GameMessage_Player_Id& msg) override;
    bool OnGameMessage(const GameMessage_Player_List& msg) override;
    bool OnGameMessage(const GameMessage_Player_Set_State& msg) override;
    bool OnGameMessage(const GameMessage_Player_Set_Nation& msg) override;
    bool OnGameMessage(const GameMessage_Player_Set_Team& msg) override;
    bool OnGameMessage(const GameMessage_Player_Set_Color& msg) override;
    bool OnGameMessage(const GameMessage_Player_Kicked& msg) override;
    bool OnGameMessage(const GameMessage_Player_Ping& msg) override;
    bool OnGameMessage(const GameMessage_Player_New& msg) override;
    bool OnGameMessage(const GameMessage_Player_Ready& msg) override;
    bool OnGameMessage(const GameMessage_Player_Swap& msg) override;

    bool OnGameMessage(const GameMessage_Map_Info& msg) override;
    bool OnGameMessage(const GameMessage_Map_Data& msg) override;
    bool OnGameMessage(const GameMessage_Map_ChecksumOK& msg) override;

    bool OnGameMessage(const GameMessage_Pause& msg) override;
    bool OnGameMessage(const GameMessage_Server_NWFDone& msg) override;
    bool OnGameMessage(const GameMessage_GameCommand& msg) override;

    bool OnGameMessage(const GameMessage_GGSChange& msg) override;
    bool OnGameMessage(const GameMessage_RemoveLua& msg) override;

    bool OnGameMessage(const GameMessage_GetAsyncLog& msg) override;

    /// Wird aufgerufen, wenn der Server gegangen ist (Verbindung verloren, ungültige Nachricht etc.)
    void ServerLost();

    // Replaymethoden

    /// Schreibt den Header der Replaydatei
    void StartReplayRecording(const unsigned random_init);
    void WritePlayerInfo(SavedFile& file);

public:
    /// Virtuelle Werte der Einstellungsfenster, die aber noch nicht wirksam sind, nur um die Verzögerungen zu verstecken
    // TODO: Move to viewer
    VisualSettings visual_settings, default_settings; //-V730_NOINIT
    /// skip ahead how many gf?
    unsigned skiptogf;

private:
    /// Spieler-ID dieses Clients
    unsigned playerId_;

    MessageQueue recv_queue, send_queue;
    Socket socket;

    ClientState state;

    /// Gameworld and event manager (valid during LOADING and GAME state)
    boost::shared_ptr<Game> game;
    /// Game lobby (valid during CONFIG state)
    boost::shared_ptr<GameLobby> gameLobby;

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

    boost::interprocess::unique_ptr<AIPlayer, Deleter<AIPlayer> > human_ai;

    /// GameCommands, die vom Client noch an den Server gesendet werden müssen
    std::vector<gc::GameCommandPtr> gameCommands_;

    boost::interprocess::unique_ptr<ReplayInfo, Deleter<ReplayInfo> > replayinfo;
    bool replayMode;
};

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#define GAMECLIENT GameClient::inst()

#endif
