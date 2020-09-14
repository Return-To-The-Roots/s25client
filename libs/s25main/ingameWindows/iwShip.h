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

#pragma once

#include "IngameWindow.h"

class noShip;
class GameWorldView;
class GameCommandFactory;

class iwShip : public IngameWindow
{
    GameWorldView& gwv;
    GameCommandFactory& gcFactory;
    unsigned char player; /// Besitzer des Schiffes, den wir für die Umwandlung ID->richtiges Schiff brauchen
    unsigned ship_id;     /// ID des Schiffes, welches gerade angezeigt wird

public:
    iwShip(GameWorldView& gwv, GameCommandFactory& gcFactory, const noShip* ship, const DrawPoint& pos = IngameWindow::posAtMouse);

private:
    void Draw_() override;
    void Msg_ButtonClick(unsigned ctrl_id) override;

    void DrawCargo();
};
