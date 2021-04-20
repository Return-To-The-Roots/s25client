// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "PreviewMinimap.h"
#include "ctrlMinimap.h"
#include "gameTypes/MapCoordinates.h"
#include "gameData/MaxPlayers.h"
#include <array>
class Window;

/// Übersichtskarte (MapPreview)
class ctrlPreviewMinimap : public ctrlMinimap
{
    /// Minimap
    PreviewMinimap minimap;
    /// Startpositionen der Spieler
    struct Player
    {
        Player();
        /// Map-Koordinaten der Startposition
        MapPoint pos;
        /// Farbe
        unsigned color;
    };
    std::array<Player, MAX_PLAYERS> players;

public:
    ctrlPreviewMinimap(Window* parent, unsigned id, const DrawPoint& pos, const Extent& size,
                       libsiedler2::ArchivItem_Map* s2map);

    /// Zeichnet die MapPreview
    void Draw_() override;
    Rect GetBoundaryRect() const override;

    /// Setzt die (Start-)Farbe eines Spielers bzw. löscht diesen (color = 0)
    void SetPlayerColor(unsigned id, unsigned color) { players[id].color = color; }

    void SetMap(const libsiedler2::ArchivItem_Map* s2map);
};
