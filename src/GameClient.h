// $Id: GameClient.h 9540 2014-12-14 11:32:47Z marcus $
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
#ifndef GAMECLIENT_H_
#define GAMECLIENT_H_

#include "Singleton.h"
#include "Socket.h"
#include "BinaryFile.h"

#include "GameMessageInterface.h"

#include "GamePlayerList.h"

#include "EventManager.h"
#include "GameSavegame.h"
#include "GameReplay.h"
#include "GameWorld.h"
#include "GlobalGameSettings.h"
#include "AIEventManager.h"

class Window;
class GameClientPlayer;
class WorldManager;
class ClientInterface;
class GameMessage;
class AIBase;

namespace gc { class GameCommand; }

class GameClient : public Singleton<GameClient>, public GameMessageInterface
{
    public:
        enum ClientState
        {
            CS_STOPPED = 0,
            CS_CONNECT,
            CS_CONFIG,
            CS_GAME
        };

        GameClient(void);
        ~GameClient(void);

        void SetInterface(ClientInterface* ci) { this->ci = ci; }
        bool IsHost() const { return clientconfig.host; }
        bool IsSavegame() const { return mapinfo.map_type == MAPTYPE_SAVEGAME; }
        std::string GetGameName() const { return clientconfig.gamename; }

        inline unsigned char GetPlayerID() const { return playerid; }
        inline unsigned GetPlayerCount() const { return players.getCount(); }
        /// Liefert einen Player zurück
        inline GameClientPlayer* GetPlayer(const unsigned int id) { return players.getElement(id); }
        inline GameClientPlayer* GetLocalPlayer(void) { return GetPlayer(playerid); }
        /// Erzeugt einen KI-Player, der mit den Daten vom GameClient gefüttert werden muss (zusätzlich noch mit den GameServer)
        AIBase* CreateAIPlayer(const unsigned playerid);

        /// Gibt GGS zurück
        const GlobalGameSettings& GetGGS() const { return ggs; }
        void LoadGGS();

        bool Connect(const std::string& server, const std::string& password, unsigned char servertyp, unsigned short port, bool host, bool use_ipv6);
        void Run();
        void Stop();

        // Gibt GameWorldViewer zurück (VORLÄUFIG, soll später verschwinden!!)
        GameWorldViewer* QueryGameWorldViewer() const { return static_cast<GameWorldViewer*>(gw); }
        /// Gibt Map-Titel zurück
        const std::string& GetMapTitle() const { return mapinfo.title; }
        /// Gibt Pfad zu der Map zurück
        const std::string& GetMapPath() const  { return clientconfig.mapfilepath; }
        /// Gibt Map-Typ zurück
        const MapType GetMapType() const { return mapinfo.map_type; }

        // Initialisiert und startet das Spiel
        void StartGame(const unsigned random_init);
        /// Wird aufgerufen, wenn das GUI fertig mit Laden ist und es losgehen kann
        void RealStart();

        /// Beendet das Spiel, zerstört die Spielstrukturen
        void ExitGame();

        ClientState GetState() const { return state; }
        inline unsigned int GetGFNumber() const { return framesinfo.nr; }
        inline unsigned int GetGFLength() const { return framesinfo.gf_length; }
        inline unsigned int GetNWFLength() const { return framesinfo.nwf_length; }
        inline unsigned int GetFrameTime() const { return framesinfo.frame_time; }
        unsigned int GetGlobalAnimation(const unsigned short max, const unsigned char factor_numerator, const unsigned char factor_denumerator, const unsigned int offset);
        unsigned int Interpolate(unsigned max_val, EventManager::EventPointer ev);
        int Interpolate(int x1, int x2, EventManager::EventPointer ev);
        /// Gibt Geschwindigkeits-Faktor zurück

        /// Fügt ein GameCommand für den Spieler hinzu und gibt bei Erfolg true zurück, ansonstn false (in der Pause oder wenn Spieler besiegt ist)
        bool AddGC(gc::GameCommand* gc);

        void Command_SetFlag2(int x, int y, unsigned char player);
        void Command_Chat(const std::string& text, const ChatDestination cd );
        void Command_ToggleNation();
        void Command_ToggleTeam(Team newteam);
        void Command_ToggleColor();
        void Command_ToggleReady();

        void IncreaseSpeed();

        /// Lädt ein Replay und startet dementsprechend das Spiel (0 = alles OK, alles andere entsprechende Fehler-ID!)
        unsigned StartReplay(const std::string& path, GameWorldViewer*& gwv);
        /// Replay-Geschwindigkeit erhöhen/verringern
        void IncreaseReplaySpeed();
        void DecreaseReplaySpeed();
        void SetReplayPause(bool pause);
        void ToggleReplayPause() { SetReplayPause(!framesinfo.pause); }
        /// Schaltet FoW im Replaymodus ein/aus
        void ToggleReplayFOW() { replayinfo.all_visible = !replayinfo.all_visible; }
        /// Prüft, ob FoW im Replaymodus ausgeschalten ist
        bool IsReplayFOWDisabled() const { return replayinfo.all_visible; }
        /// Gibt Replay-Ende (GF) zurück
        unsigned GetLastReplayGF() const { return replayinfo.replay.last_gf; }
        /// Wandelt eine GF-Angabe in eine Zeitangabe um (HH:MM:SS oder MM:SS wenn Stunden = 0)
        std::string FormatGFTime(const unsigned gf) const;

        /// Gibt Replay-Dateiname zurück
        const std::string& GetReplayFileName() const { return replayinfo.filename; }
        /// Wird ein Replay abgespielt?
        bool IsReplayModeOn() const { return replay_mode; }

        const Replay GetReplay() const { return replayinfo.replay; }

        /// Is tournament mode activated (0 if not)? Returns the durations of the tournament mode in gf otherwise
        unsigned GetTournamentModeDuration() const;

        void SkipGF(unsigned int gf);

        /// Wechselt den aktuellen Spieler (nur zu Debugzwecken !!)
        void ChangePlayer(const unsigned char old_id, const unsigned char new_id);

        /// Wechselt den aktuellen Spieler im Replaymodus
        void ChangeReplayPlayer(const unsigned new_id);
        /// Laggt ein bestimmter Spieler gerade?
        bool IsLagging(const unsigned int id) { return GetPlayer(id)->is_lagging; }
        /// Spiel pausiert?
        bool IsPaused() const { return framesinfo.pause; }
        /// Schreibt Header der Save-Datei
        unsigned WriteSaveHeader(const std::string& filename);
        /// Visuelle Einstellungen aus den richtigen ableiten
        void GetVisualSettings();

        /// Schreibt ggf. Pathfinding-Results in das Replay, falls erforderlich
        void AddPathfindingResult(const unsigned char dir, const unsigned* const length, const Point<MapCoord>* const next_harbor);
        /// Gibt zurück, ob Pathfinding-Results zur Verfügung stehen
        bool ArePathfindingResultsAvailable() const;
        /// Gibt Pathfinding-Results zurück aus einem Replay
        bool ReadPathfindingResult( unsigned char* dir, unsigned* length, Point<MapCoord>* next_harbor);

        void SystemChat(std::string text);
    private:
        /// Versucht einen neuen GameFrame auszuführen, falls die Zeit dafür gekommen ist
        void ExecuteGameFrame(const bool skipping = false);
        void ExecuteGameFrame_Replay();
        void ExecuteGameFrame_Game();
        /// Filtert aus einem Network-Command-Paket alle Commands aus und führt sie aus, falls ein Spielerwechsel-Command
        /// dabei ist, füllt er die übergebenen IDs entsprechend aus
        void ExecuteAllGCs(const GameMessage_GameCommand& gcs,  unsigned char* player_switch_old_id, unsigned char* player_switch_new_id);
        /// Sendet ein NC-Paket ohne Befehle
        void SendNothingNC(int checksum = -1);
        /// Findet heraus, ob ein Spieler laggt und setzt bei diesen Spieler den entsprechenden flag
        bool IsPlayerLagging();

        /// Führt notwendige Dinge für nächsten GF aus
        void NextGF();

        /// Führt für alle Spieler einen Statistikschritt aus, wenn die Zeit es verlangt
        void StatisticStep();

        //  Netzwerknachrichten
        virtual void OnNMSDeadMsg(unsigned int id);

        virtual void OnNMSPing(const GameMessage_Ping& msg);

        virtual void OnNMSServerTypeOK(const GameMessage_Server_TypeOK& msg);
        virtual void OnNMSServerPassword(const GameMessage_Server_Password& msg);
        virtual void OnNMSServerName(const GameMessage_Server_Name& msg);
        virtual void OnNMSServerStart(const GameMessage_Server_Start& msg);
        virtual void OnNMSServerChat(const GameMessage_Server_Chat& msg);
        virtual void OnNMSServerAsync(const GameMessage_Server_Async& msg);
        virtual void OnNMSServerCountdown(const GameMessage_Server_Countdown& msg);
        virtual void OnNMSServerCancelCountdown(const GameMessage_Server_CancelCountdown& msg);

        virtual void OnNMSPlayerId(const GameMessage_Player_Id& msg);
        virtual void OnNMSPlayerList(const GameMessage_Player_List& msg);
        virtual void OnNMSPlayerToggleState(const GameMessage_Player_Toggle_State& msg);
        virtual void OnNMSPlayerToggleNation(const GameMessage_Player_Toggle_Nation& msg);
        virtual void OnNMSPlayerToggleTeam(const GameMessage_Player_Toggle_Team& msg);
        virtual void OnNMSPlayerToggleColor(const GameMessage_Player_Toggle_Color& msg);
        virtual void OnNMSPlayerKicked(const GameMessage_Player_Kicked& msg);
        virtual void OnNMSPlayerPing(const GameMessage_Player_Ping& msg);
        virtual void OnNMSPlayerNew(const GameMessage_Player_New& msg);
        virtual void OnNMSPlayerReady(const GameMessage_Player_Ready& msg);
        virtual void OnNMSPlayerSwap(const GameMessage_Player_Swap& msg);

        void OnNMSMapInfo(const GameMessage_Map_Info& msg);
        void OnNMSMapData(const GameMessage_Map_Data& msg);
        void OnNMSMapChecksumOK(const GameMessage_Map_ChecksumOK& msg);

        virtual void OnNMSPause(const GameMessage_Pause& msg);
        virtual void OnNMSServerDone(const GameMessage_Server_NWFDone& msg);
        virtual void OnNMSGameCommand(const GameMessage_GameCommand& msg);
        virtual void OnNMSServerSpeed(const GameMessage_Server_Speed& msg);

        void OnNMSGGSChange(const GameMessage_GGSChange& msg);

        void OnNMSGetAsyncLog(const GameMessage_GetAsyncLog& msg);

        /// Wird aufgerufen, wenn der Server gegangen ist (Verbindung verloren, ungültige Nachricht etc.)
        void ServerLost();

        // Replaymethoden

        /// Schreibt den Header der Replaydatei
        void WriteReplayHeader(const unsigned random_init);

// Post-Sachen
    public:

        void SendPostMessage(PostMsg* msg);
        const std::list<PostMsg*>& GetPostMessages() { return postMessages; }
        void DeletePostMessage(PostMsg* msg);

        void SendAIEvent(AIEvent::Base* ev, unsigned receiver);

    private:
        std::list<PostMsg*> postMessages;

    public:
        /// Virtuelle Werte der Einstellungsfenster, die aber noch nicht wirksam sind, nur um die Verzögerungen zu
        /// verstecken
        struct VisualSettings
        {
            /// Verteilung
            std::vector<unsigned char> distribution;
            /// Art der Reihenfolge (0 = nach Auftraggebung, ansonsten nach build_order)
            unsigned char order_type;
            /// Baureihenfolge
            std::vector<unsigned char> build_order;
            /// Transport-Reihenfolge
            std::vector<unsigned char> transport_order;
            /// Militäreinstellungen (die vom Militärmenü)
            std::vector<unsigned char> military_settings;
            /// Werkzeugeinstellungen (in der Reihenfolge wie im Fenster!)
            std::vector<unsigned char> tools_settings;

            VisualSettings() : distribution(23), build_order(31), transport_order(14), military_settings(MILITARY_SETTINGS_COUNT), tools_settings(12)
            {}

        } visual_settings, default_settings;
		/// skip ahead how many gf?
		unsigned int skiptogf;
    private:
        /// Spielwelt
        GameWorld* gw;
        /// EventManager
        EventManager* em;
        /// Spieler
        GameClientPlayerList players;
        /// Spieler-ID dieses Clients
        unsigned char playerid;
        /// Globale Spieleinstellungen
        GlobalGameSettings ggs;

        MessageQueue recv_queue, send_queue;
        Socket socket;
        // Was soll das sein? oO
        unsigned int temp_ul;
        unsigned int temp_ui;

        ClientState state;

        class ClientConfig
        {
            public:
                ClientConfig() { Clear(); }
                void Clear();

                std::string server;
                std::string gamename;
                std::string password;
                std::string mapfile;
                std::string mapfilepath;
                unsigned char servertyp;
                unsigned short port;
                bool host;
        } clientconfig;

        class MapInfo
        {
            public:
                MapInfo() { Clear(); }
                void Clear();

                MapType map_type;
                unsigned partcount;
                unsigned ziplength;
                unsigned length;
                unsigned checksum;
                std::string title;
                unsigned char* zipdata;
                Savegame savegame;
        } mapinfo;

        class FramesInfo
        {
            public:
                FramesInfo() { Clear(); }
                void Clear();

                /// Aktueller GameFrame (der wievielte seit Beginn)
                unsigned nr;
                unsigned nr_srv;
                /// Länge der GameFrames in ms (= Geschwindigkeit des Spiels)
                unsigned gf_length;
                unsigned gf_length_new;
                /// Länge der Network-Frames in gf(!)
                unsigned nwf_length;

                /// Zeit in ms seit dem letzten Frame
                unsigned frame_time;

                unsigned lasttime;
                unsigned lastmsgtime;
                unsigned pausetime;

                bool pause;
				unsigned pause_gf;
        } framesinfo;

        class RandCheckInfo
        {
            public:
                RandCheckInfo() { Clear(); }
                void Clear();

                int rand;
        } randcheckinfo;


        ClientInterface* ci;

        /// GameCommands, die vom Client noch an den Server gesendet werden müssen
        std::vector<gc::GameCommand*> gcs;

        struct ReplayInfo
        {
            public:
                ReplayInfo() { Clear(); }
                void Clear();

                /// Replaydatei
                Replay replay;
                /// Replay asynchron (Meldung nur einmal ausgeben!)
                int async;
                bool end;
                // Nächster Replay-Command-Zeitpunkt (in GF)
                unsigned next_gf;
                /// Replay-Dateiname
                std::string filename;
                /// Alles sichtbar (FoW deaktiviert)
                bool all_visible;
        } replayinfo;

        /// Replaymodus an oder aus?
        bool replay_mode;
		

        /// Spiel-Log für Asyncs
        FILE* game_log;
};

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#define GAMECLIENT GameClient::inst()

#endif
