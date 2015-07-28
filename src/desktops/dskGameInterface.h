﻿// $Id: dskGameInterface.h 9357 2014-04-25 15:35:25Z FloSoft $
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
#ifndef dskGAMEINTERFACE_H_INCLUDED
#define dskGAMEINTERFACE_H_INCLUDED

#pragma once

#include "Desktop.h"
#include "Messenger.h"
#include "ingameWindows/iwAction.h"
#include "gameTypes/MapTypes.h"
#include "Minimap.h"
#include "customborderbuilder.h"

#include "ClientInterface.h"
#include "GameInterface.h"
#include "LobbyInterface.h"

class IngameWindow;
class iwAction;
class iwRoadWindow;
class GameWorldViewer;

enum RoadMode
{
    RM_DISABLED, // kein Straßenbau
    RM_NORMAL, // Bau einer normalen Straße
    RM_BOAT // Bau einer Wasserstraße
};

struct RoadsBuilding
{
    RoadMode mode;   ///< Straßenmodus

    MapPoint point, start;
    std::vector<unsigned char> route;  ///< Richtungen der gebauten Straße
};

class dskGameInterface :
    public Desktop,
    public ClientInterface,
    public GameInterface,
    public LobbyInterface
{
    private:

        // Interface für das Spiel
        GameWorldViewer* gwv;

        CustomBorderBuilder cbb;

        libsiedler2::ArchivInfo borders;

        /// Straßenbauzeug
        RoadsBuilding road;

        // Aktuell geöffnetes Aktionsfenster
        iwAction* actionwindow;
        // Aktuell geöffnetes Straßenbaufenster
        iwRoadWindow* roadwindow;
        // Is der lokale Spieler der Host?
        bool ishost;
        // Messenger für die Nachrichten
        Messenger messenger;
        // Aktuell selektierter Punkt auf der Karte
        MapPoint selected;
        /// Minimap-Instanz
        IngameMinimap minimap;

    public:
        /// Konstruktor von @p dskGameInterface.
        dskGameInterface();
        /// Destruktor von @p dskGameInterface.
        ~dskGameInterface(void);

        void LC_Status_ConnectionLost(void);
        void LC_Status_Error(const std::string& error);
        /// Called whenever Settings are changed ingame
        void SettingsChanged(void);

        /// Lässt das Spiel laufen (zeichnen)
        void Run();

        /// Wird aufgerufen, wenn eine Taste gedrückt wurde
        void KeyPressed(KeyEvent ke);
        /// Wird bei Linksmausklicks aufgerufen
        void MouseClicked(MouseCoords* mc);

        /// Aktiviert Straßenbaumodus bzw gibt zurück, ob er aktiviert ist
        void ActivateRoadMode(const RoadMode rm);
        RoadMode GetRoadMode() const { return road.mode; }

        /// Baut die gewünschte bis jetzt noch visuelle Straße (schickt Anfrage an Server)
        void CommandBuildRoad();

        /// Wird aufgerufen, wenn die Fenster geschlossen werden
        void ActionWindowClosed();
        void RoadWindowClosed();

        friend class GameClient;
        friend class GameWorld;
        friend class RoadSegment;

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

        void Resize_(unsigned short width, unsigned short height);

        /// Baut Weg zurück von Ende bis zu start_id
        void DemolishRoad(const unsigned start_id);

        /// Updatet das Post-Icon mit der Nachrichtenanzahl und der Taube
        void UpdatePostIcon(const unsigned postmessages_count, bool showPigeon);

        void Msg_ButtonClick(const unsigned int ctrl_id);
        void Msg_PaintBefore();
        void Msg_PaintAfter();
        bool Msg_LeftDown(const MouseCoords& mc);
        bool Msg_LeftUp(const MouseCoords& mc);
        bool Msg_MouseMove(const MouseCoords& mc);
        bool Msg_RightDown(const MouseCoords& mc);
        bool Msg_RightUp(const MouseCoords& mc);
        bool Msg_KeyDown(const KeyEvent& ke);

        void CI_PlayerLeft(const unsigned player_id);
        void CI_GGSChanged(const GlobalGameSettings& ggs);
        void CI_Chat(const unsigned player_id, const ChatDestination cd, const std::string& msg);
        void CI_Async(const std::string& checksums_list);
        void CI_ReplayAsync(const std::string& msg);
        void CI_ReplayEndReached(const std::string& msg);
        void CI_GamePaused();
        void CI_GameResumed();
        void CI_Error(const ClientError ce);
        void CI_NewPostMessage(const unsigned postmessages_count);
        void CI_PostMessageDeleted(const unsigned postmessages_count);

        /// Wird aufgerufen, wann immer eine Flagge zerstört wurde, da so evtl der Wegbau abgebrochen werden muss
        void GI_FlagDestroyed(const MapPoint pt);
        /// Spielerwechsel
        void CI_PlayersSwapped(const unsigned player1, const unsigned player2);

        /// Wenn ein Spieler verloren hat
        void GI_PlayerDefeated(const unsigned player_id);
        /// Es wurde etwas Minimap entscheidendes geändert --> Minimap updaten
        void GI_UpdateMinimap(const MapPoint pt);
        /// Bündnisvertrag wurde abgeschlossen oder abgebrochen --> Minimap updaten
        void GI_TreatyOfAllianceChanged();

        void GI_Winner(const unsigned player_id);
        void GI_TeamWinner(const unsigned player_id);
};

#endif // !dskGAMEINTERFACE_H_INCLUDED
