// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "gameTypes/MapCoordinates.h"

class Cheats;

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

    /// Changes into road building mode
    virtual void GI_StartRoadBuilding(MapPoint startPt, bool waterRoad) = 0;
    /// Cancels the road building mode
    virtual void GI_CancelRoadBuilding() = 0;
    /// Executes the construction of a road
    virtual void GI_BuildRoad() = 0;

    virtual Cheats& GI_GetCheats() = 0;
};
