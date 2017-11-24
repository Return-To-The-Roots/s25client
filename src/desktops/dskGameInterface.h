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
#ifndef dskGAMEINTERFACE_H_INCLUDED
#define dskGAMEINTERFACE_H_INCLUDED

#pragma once

#include "Desktop.h"
#include "GameInterface.h"
#include "IngameMinimap.h"
#include "Messenger.h"
#include "customborderbuilder.h"
#include "ingameWindows/iwAction.h"
#include "network/ClientInterface.h"
#include "notifications/Subscribtion.h"
#include "world/GameWorldView.h"
#include "world/GameWorldViewer.h"
#include "gameTypes/MapCoordinates.h"
#include "gameTypes/RoadBuildState.h"
#include "liblobby/LobbyInterface.h"
#include <boost/array.hpp>

class iwRoadWindow;
class glArchivItem_Bitmap;
class GlobalGameSettings;
class MouseCoords;
class GameWorldBase;
class PostBox;
class PostMsg;
struct BuildingNote;
struct KeyEvent;
struct ClientPlayers;

class dskGameInterface : public Desktop, public ClientInterface, public GameInterface, public LobbyInterface
{
public:
    dskGameInterface(boost::shared_ptr<Game> game);
    ~dskGameInterface() override;

    void Resize(const Extent& newSize) override;
    void SetActive(bool activate = true) override;

    void LC_Status_ConnectionLost() override;
    void LC_Status_Error(const std::string& error) override;
    /// Called whenever Settings are changed ingame
    void SettingsChanged();

    RoadBuildMode GetRoadMode() const { return road.mode; }

    void CI_PlayerLeft(const unsigned playerId) override;
    void CI_GGSChanged(const GlobalGameSettings& ggs) override;
    void CI_Chat(const unsigned playerId, const ChatDestination cd, const std::string& msg) override;
    void CI_Async(const std::string& checksums_list) override;
    void CI_ReplayAsync(const std::string& msg) override;
    void CI_ReplayEndReached(const std::string& msg) override;
    void CI_GamePaused() override;
    void CI_GameResumed() override;
    void CI_Error(const ClientError ce) override;
    void CI_PlayersSwapped(const unsigned player1, const unsigned player2) override;

    void NewPostMessage(const PostMsg& msg, unsigned msgCt);
    void PostMessageDeleted(unsigned msgCt);

    /// Wird aufgerufen, wann immer eine Flagge zerstört wurde, da so evtl der Wegbau abgebrochen werden muss
    void GI_FlagDestroyed(const MapPoint pt) override;
    /// Wenn ein Spieler verloren hat
    void GI_PlayerDefeated(unsigned playerId) override;
    /// Es wurde etwas Minimap entscheidendes geändert --> Minimap updaten
    void GI_UpdateMinimap(const MapPoint pt) override;
    /// Bündnisvertrag wurde abgeschlossen oder abgebrochen --> Minimap updaten
    void GI_TreatyOfAllianceChanged(unsigned playerId) override;
    void GI_Winner(const unsigned playerId) override;
    void GI_TeamWinner(const unsigned playerId) override;
    void GI_StartRoadBuilding(const MapPoint startPt, bool waterRoad) override;
    void GI_CancelRoadBuilding() override;
    /// Baut die gewünschte bis jetzt noch visuelle Straße (schickt Anfrage an Server)
    void GI_BuildRoad() override;
    void GI_WindowClosed(Window* wnd) override;

    // Sucht einen Weg von road_point_x/y zu cselx/y und baut ihn ( nur visuell )
    // Bei Wasserwegen kann die Reichweite nicht bis zum gewünschten
    // Punkt reichen. Dann werden die Zielkoordinaten geändert, daher
    // call-by-reference
    bool BuildRoadPart(MapPoint& cSel);
    // Return the id (index + 1) of the point in the currently build road (1 = startPt)
    // If pt is not on the road, return 0
    unsigned GetIdInCurBuildRoad(const MapPoint pt);
    /// Baut Weg zurück von Ende bis zu start_id
    void DemolishRoad(const unsigned start_id);
    // Zeigt das Straäcnfenster an und entscheidet selbststäcdig, ob man eine Flagge an road_point_x/y bauen kann,
    // ansonsten gibt's nur nen Button zum Abbrechen
    void ShowRoadWindow(const DrawPoint& mousePos);
    /// Zeigt das Actionwindow an, bei Flaggen werden z.B. noch berücksichtigt, obs ne besondere Flagge ist usw
    void ShowActionWindow(const iwAction::Tabs& action_tabs, MapPoint cSel, const DrawPoint& mousePos,
                          const bool enable_military_buildings);

    const GameWorldViewer& GetViewer() const { return worldViewer; }

private:
    /// Initializes player specific stuff after start or player swap
    void InitPlayer();

    /// Lässt das Spiel laufen (zeichnen)
    void Run();

    /// Updatet das Post-Icon mit der Nachrichtenanzahl und der Taube
    void UpdatePostIcon(const unsigned postmessages_count, bool showPigeon);

    void Msg_ButtonClick(const unsigned ctrl_id) override;
    void Msg_PaintBefore() override;
    void Msg_PaintAfter() override;
    bool Msg_LeftDown(const MouseCoords& mc) override;
    bool Msg_LeftUp(const MouseCoords& mc) override;
    bool Msg_MouseMove(const MouseCoords& mc) override;
    bool Msg_RightDown(const MouseCoords& mc) override;
    bool Msg_RightUp(const MouseCoords& mc) override;
    bool Msg_KeyDown(const KeyEvent& ke) override;

    bool Msg_WheelUp(const MouseCoords& mc) override;
    bool Msg_WheelDown(const MouseCoords& mc) override;
    void WheelZoom(float step);

    void OnBuildingNote(const BuildingNote& note);

    PostBox& GetPostBox();
    boost::shared_ptr<const Game> game_;
    boost::shared_ptr<const ClientPlayers> networkPlayers;
    GameWorldViewer worldViewer;
    GameWorldView gwv;

    CustomBorderBuilder cbb;

    boost::array<glArchivItem_Bitmap*, 4> borders;

    /// Straßenbauzeug
    RoadBuildState road;

    // Aktuell geöffnetes Aktionsfenster
    iwAction* actionwindow;
    // Aktuell geöffnetes Straßenbaufenster
    iwRoadWindow* roadwindow;
    // Messenger für die Nachrichten
    Messenger messenger;
    /// Minimap-Instanz
    IngameMinimap minimap;

    bool isScrolling;
    Position startScrollPt;
    size_t zoomLvl;
    bool isCheatModeOn;
    std::string curCheatTxt;
    Subscribtion evBld;
};

#endif // !dskGAMEINTERFACE_H_INCLUDED
