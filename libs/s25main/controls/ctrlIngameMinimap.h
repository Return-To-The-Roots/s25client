// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "ctrlMinimap.h"

class GameWorldView;
class IngameMinimap;
class MouseCoords;
class Window;

/// Minimap-Control für Ingame
class ctrlIngameMinimap : public ctrlMinimap
{
    /// Zeiger auf Minimap (die im Spiel dauerhaft!! gespeichert werden muss)
    IngameMinimap& minimap;
    /// Referenz auf GameWorldView, für das Gescrolle
    GameWorldView& gwv;

public:
    ctrlIngameMinimap(Window* parent, unsigned id, const DrawPoint& pos, const Extent& size, const Extent& padding,
                      IngameMinimap& minimap, GameWorldView& gwv);

    /// Zeichnet die MapPreview
    void Draw_() override;

    bool Msg_LeftDown(const MouseCoords& mc) override;
    bool Msg_MouseMove(const MouseCoords& mc) override;

    /// Die einzelnen Dinge umschalten
    void ToggleTerritory();
    void ToggleHouses();
    void ToggleRoads();
};
