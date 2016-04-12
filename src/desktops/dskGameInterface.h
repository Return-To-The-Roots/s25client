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
#ifndef dskGAMEINTERFACE_H_INCLUDED
#define dskGAMEINTERFACE_H_INCLUDED

#pragma once

#include "Desktop.h"
#include "Messenger.h"
#include "ingameWindows/iwAction.h"
#include "IngameMinimap.h"
#include "customborderbuilder.h"
#include "ClientInterface.h"
#include "GameInterface.h"
#include "LobbyInterface.h"
#include "world/GameWorldView.h"
#include "gameTypes/MapTypes.h"
#include "gameTypes/RoadBuildState.h"
#include "libsiedler2/src/ArchivInfo.h"

class iwRoadWindow;
class GlobalGameSettings;
class MouseCoords;
class GameWorldBase;
struct KeyEvent;

class dskGameInterface :
    public Desktop,
    public ClientInterface,
    public GameInterface,
    public LobbyInterface
{
    private:

        GameWorldView gwv;
        GameWorldBase& gwb;

        CustomBorderBuilder cbb;

        libsiedler2::ArchivInfo borders;

        /// Straßenbauzeug
        RoadBuildState road;

        // Aktuell geöffnetes Aktionsfenster
        iwAction* actionwindow;
        // Aktuell geöffnetes Straßenbaufenster
        iwRoadWindow* roadwindow;
        // Messenger für die Nachrichten
        Messenger messenger;
        // Aktuell selektierter Punkt auf der Karte
        MapPoint selected;
        /// Minimap-Instanz
        IngameMinimap minimap;

        bool isScrolling;
        Point<int> startScrollPt;
        unsigned zoomLvl;
    public:
        dskGameInterface();
        ~dskGameInterface() override;

        virtual void SetActive(bool activate = true) override;

        void LC_Status_ConnectionLost() override;
        void LC_Status_Error(const std::string& error) override;
        /// Called whenever Settings are changed ingame
        void SettingsChanged();

        RoadBuildMode GetRoadMode() const { return road.mode; }

        void CI_PlayerLeft(const unsigned player_id) override;
        void CI_GGSChanged(const GlobalGameSettings& ggs) override;
        void CI_Chat(const unsigned player_id, const ChatDestination cd, const std::string& msg) override;
        void CI_Async(const std::string& checksums_list) override;
        void CI_ReplayAsync(const std::string& msg) override;
        void CI_ReplayEndReached(const std::string& msg) override;
        void CI_GamePaused() override;
        void CI_GameResumed() override;
        void CI_Error(const ClientError ce) override;
        void CI_NewPostMessage(const unsigned postmessages_count) override;
        void CI_PostMessageDeleted(const unsigned postmessages_count) override;
        void CI_PlayersSwapped(const unsigned player1, const unsigned player2) override;

        /// Wird aufgerufen, wann immer eine Flagge zerstört wurde, da so evtl der Wegbau abgebrochen werden muss
        void GI_FlagDestroyed(const MapPoint pt) override;
        /// Wenn ein Spieler verloren hat
        void GI_PlayerDefeated(const unsigned player_id) override;
        /// Es wurde etwas Minimap entscheidendes geändert --> Minimap updaten
        void GI_UpdateMinimap(const MapPoint pt) override;
        /// Bündnisvertrag wurde abgeschlossen oder abgebrochen --> Minimap updaten
        void GI_TreatyOfAllianceChanged() override;
        void GI_Winner(const unsigned player_id) override;
        void GI_TeamWinner(const unsigned player_id) override;
        void GI_SetRoadBuildMode(RoadBuildMode mode) override;
        /// Baut die gewünschte bis jetzt noch visuelle Straße (schickt Anfrage an Server)
        void GI_BuildRoad() override;
        void GI_WindowClosed(Window* wnd) override;

        // Sucht einen Weg von road_point_x/y zu cselx/y und baut ihn ( nur visuell )
        // Bei Wasserwegen kann die Reichweite nicht bis zum gewünschten
        // Punkt reichen. Dann werden die Zielkoordinaten geändert, daher
        // call-by-reference
        bool BuildRoadPart(MapPoint& cSel, bool end);
        // Prft, ob x;y auf der bereits gebauten Strecke liegt und gibt die Position+1 zurck vom Startpunkt der Strecke aus
        // wenn der Punkt nicht draufliegt, kommt 0 zurck
        unsigned TestBuiltRoad(const MapPoint pt);
        // Zeigt das Straäcnfenster an und entscheidet selbststäcdig, ob man eine Flagge an road_point_x/y bauen kann,
        // ansonsten gibt's nur nen Button zum Abbrechen
        void ShowRoadWindow(int mouse_x, int mouse_y);
        /// Zeigt das Actionwindow an, bei Flaggen werden z.B. noch berücksichtigt, obs ne besondere Flagge ist usw
        void ShowActionWindow(const iwAction::Tabs& action_tabs, MapPoint cSel, int mouse_x, int mouse_y, const bool enable_military_buildings);

    private:
        /// Lässt das Spiel laufen (zeichnen)
        void Run();

        void Resize_(unsigned short width, unsigned short height) override;

        /// Baut Weg zurück von Ende bis zu start_id
        void DemolishRoad(const unsigned start_id);

        /// Updatet das Post-Icon mit der Nachrichtenanzahl und der Taube
        void UpdatePostIcon(const unsigned postmessages_count, bool showPigeon);

        void Msg_ButtonClick(const unsigned int ctrl_id) override;
        void Msg_PaintBefore() override;
        void Msg_PaintAfter() override;
        bool Msg_LeftDown(const MouseCoords& mc) override;
        bool Msg_LeftUp(const MouseCoords& mc) override;
        bool Msg_MouseMove(const MouseCoords& mc) override;
        bool Msg_RightDown(const MouseCoords& mc) override;
        bool Msg_RightUp(const MouseCoords& mc) override;
        bool Msg_KeyDown(const KeyEvent& ke) override;
};

#endif // !dskGAMEINTERFACE_H_INCLUDED
