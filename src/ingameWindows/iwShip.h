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
#ifndef iwSHIP_H_INCLUDED
#define iwSHIP_H_INCLUDED

#pragma once

#include "IngameWindow.h"

class noShip;
class GameWorldView;

class iwShip : public IngameWindow
{
        GameWorldView& gwv;
        unsigned ship_id; /// ID des Schiffes, welches gerade angezeigt wird
        unsigned char player; /// Besitzer des Schiffes, den wir für die Umwandlung ID->richtiges Schiff brauchen

    public:
        iwShip(GameWorldView& gwv, noShip* const ship);

    private:

        void Msg_PaintBefore() override;
        void Msg_PaintAfter() override;
        void Msg_ButtonClick(const unsigned int ctrl_id) override;

        void DrawCargo();
};

#endif // !iwSHIP_H_INCLUDED
