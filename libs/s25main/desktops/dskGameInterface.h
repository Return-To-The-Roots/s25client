// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "Desktop.h"
#include "GameInterface.h"
#include "IngameMinimap.h"
#include "Messenger.h"
#include "customborderbuilder.h"
#include "ingameWindows/iwAction.h"
#include "ingameWindows/iwChat.h"
#include "network/ClientInterface.h"
#include "notifications/Subscription.h"
#include "world/GameWorldView.h"
#include "world/GameWorldViewer.h"
#include "gameTypes/MapCoordinates.h"
#include "gameTypes/RoadBuildState.h"
#include "liblobby/LobbyInterface.h"
#include <array>

class IngameWindow;
class glArchivItem_Bitmap;
class GlobalGameSettings;
class MouseCoords;
class PostBox;
class PostMsg;
struct BuildingNote;
struct KeyEvent;
class NWFInfo;

class dskGameInterface :
    public Desktop,
    public ClientInterface,
    public GameInterface,
    public LobbyInterface,
    public IChatCmdListener
{
public:
    dskGameInterface(std::shared_ptr<Game> game, std::shared_ptr<const NWFInfo> nwfInfo, unsigned playerIdx,
                     bool initOGL = true);
    ~dskGameInterface() override;

    void Resize(const Extent& newSize) override;
    void SetActive(bool activate = true) override;

    void LC_Status_ConnectionLost() override;
    void LC_Status_Error(const std::string& error) override;

    RoadBuildMode GetRoadMode() const { return road.mode; }

    void CI_PlayerLeft(unsigned playerId) override;
    void CI_GGSChanged(const GlobalGameSettings& ggs) override;
    void CI_Chat(unsigned playerId, ChatDestination cd, const std::string& msg) override;
    void CI_Async(const std::string& checksums_list) override;
    void CI_ReplayAsync(const std::string& msg) override;
    void CI_ReplayEndReached(const std::string& msg) override;
    void CI_GamePaused() override;
    void CI_GameResumed() override;
    void CI_Error(ClientError ce) override;
    void CI_PlayersSwapped(unsigned player1, unsigned player2) override;

    void NewPostMessage(const PostMsg& msg, unsigned msgCt);
    void PostMessageDeleted(unsigned msgCt);

    /// Wird aufgerufen, wann immer eine Flagge zerstört wurde, da so evtl der Wegbau abgebrochen werden muss
    void GI_FlagDestroyed(MapPoint pt) override;
    /// Wenn ein Spieler verloren hat
    void GI_PlayerDefeated(unsigned playerId) override;
    /// Es wurde etwas Minimap entscheidendes geändert --> Minimap updaten
    void GI_UpdateMinimap(MapPoint pt) override;
    /// Update minimap and colors for whole map
    void GI_UpdateMapVisibility() override;

    /// Bündnisvertrag wurde abgeschlossen oder abgebrochen --> Minimap updaten
    void GI_TreatyOfAllianceChanged(unsigned playerId) override;
    void GI_Winner(unsigned playerId) override;
    void GI_TeamWinner(unsigned playerMask) override;
    void GI_StartRoadBuilding(MapPoint startPt, bool waterRoad) override;
    void GI_CancelRoadBuilding() override;
    /// Baut die gewünschte bis jetzt noch visuelle Straße (schickt Anfrage an Server)
    void GI_BuildRoad() override;

    // Sucht einen Weg von road_point_x/y zu cselx/y und baut ihn ( nur visuell )
    // Bei Wasserwegen kann die Reichweite nicht bis zum gewünschten
    // Punkt reichen. Dann werden die Zielkoordinaten geändert, daher
    // call-by-reference
    bool BuildRoadPart(MapPoint& cSel);
    // Return the id (index + 1) of the point in the currently build road (1 = startPt)
    // If pt is not on the road, return 0
    unsigned GetIdInCurBuildRoad(MapPoint pt);
    /// Baut Weg zurück von Ende bis zu start_id
    void DemolishRoad(unsigned start_id);
    // Zeigt das Straäcnfenster an und entscheidet selbststäcdig, ob man eine Flagge an road_point_x/y bauen kann,
    // ansonsten gibt's nur nen Button zum Abbrechen
    void ShowRoadWindow(const Position& mousePos);
    /// Zeigt das Actionwindow an, bei Flaggen werden z.B. noch berücksichtigt, obs ne besondere Flagge ist usw
    void ShowActionWindow(const iwAction::Tabs& action_tabs, MapPoint cSel, const DrawPoint& mousePos,
                          bool enable_military_buildings);

    const GameWorldView& GetView() const { return gwv; }

    void OnChatCommand(const std::string& cmd) override;

protected:
    /// Initializes player specific stuff after start or player swap
    void InitPlayer();

    /// Lässt das Spiel laufen (zeichnen)
    void Run();

    /// Updatet das Post-Icon mit der Nachrichtenanzahl und der Taube
    void UpdatePostIcon(unsigned postmessages_count, bool showPigeon);

    void Msg_ButtonClick(unsigned ctrl_id) override;
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

    void Msg_WindowClosed(IngameWindow& wnd) override;

    void OnBuildingNote(const BuildingNote& note);

    void StopScrolling();
    void StartScrolling(const Position& mousePos);
    void ToggleFoW();              // Switch Fog of War mode if possible
    void DisableFoW(bool hideFOW); // Set Fog of War mode if possible
    void ShowPersistentWindowsAfterSwitch();

    PostBox& GetPostBox();
    std::shared_ptr<const Game> game_;
    std::shared_ptr<const NWFInfo> nwfInfo_;
    GameWorldViewer worldViewer;
    GameWorldView gwv;

    CustomBorderBuilder cbb;

    std::array<glArchivItem_Bitmap*, 4> borders;

    /// Straßenbauzeug
    RoadBuildState road;

    // Aktuell geöffnetes Aktionsfenster
    iwAction* actionwindow;
    // Aktuell geöffnetes Straßenbaufenster
    IngameWindow* roadwindow;
    // Messenger für die Nachrichten
    Messenger messenger;
    /// Minimap-Instanz
    IngameMinimap minimap;

    bool isScrolling;
    Position startScrollPt;
    size_t zoomLvl;
    bool isCheatModeOn;
    std::string curCheatTxt;
    Subscription evBld;
};
