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
#ifndef CTRL_INGAMEMINIMAP_H_
#define CTRL_INGAMEMINIMAP_H_

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
    ctrlIngameMinimap(Window* parent, unsigned id, const DrawPoint& pos, const Extent& size, const Extent& padding, IngameMinimap& minimap,
                      GameWorldView& gwv);

    /// Zeichnet die MapPreview
    void Draw_() override;

    bool Msg_LeftDown(const MouseCoords& mc) override;
    bool Msg_MouseMove(const MouseCoords& mc) override;

    /// Die einzelnen Dinge umschalten
    void ToggleTerritory();
    void ToggleHouses();
    void ToggleRoads();
};

#endif
