// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "gameTypes/MapCoordinates.h"

class Window;

/// Interface, welches vom Spiel angesprocehn werden kann, um beispielsweise GUI wichtige Nachrichten
/// zu übermiteln
class GameInterface
{
public:
    virtual ~GameInterface() = default;

    /// Ein Spieler hat verloren
    virtual void GI_PlayerDefeated(unsigned playerId) = 0;
    /// Es wurde etwas Minimap entscheidendes geändert --> Minimap updaten
    virtual void GI_UpdateMinimap(MapPoint pt) = 0;
    /// Flagge wurde zerstört
    virtual void GI_FlagDestroyed(MapPoint pt) = 0;
    /// Bündnisvertrag wurde abgeschlossen oder abgebrochen --> Minimap updaten
    virtual void GI_TreatyOfAllianceChanged(unsigned playerId) = 0;

    /// Update minimap and colors for whole map
    virtual void GI_UpdateMapVisibility() {}

    virtual void GI_Winner(unsigned playerId) = 0;
    virtual void GI_TeamWinner(unsigned playerId) = 0;

    /// An important window was closed (currently iwAction, iwRoad)
    virtual void GI_WindowClosed(Window* wnd) = 0;
    /// Changes into road building mode
    virtual void GI_StartRoadBuilding(MapPoint startPt, bool waterRoad) = 0;
    /// Cancels the road building mode
    virtual void GI_CancelRoadBuilding() = 0;
    /// Executes the construction of a road
    virtual void GI_BuildRoad() = 0;
};
