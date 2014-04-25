// $Id: iwDemolishBuilding.h
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
#ifndef iwOBSERVATE_H_INCLUDED
#define iwOBSERVATE_H_INCLUDED

#pragma once

#include "IngameWindow.h"
#include "GameConsts.h"
#include "GameObject.h"
#include "GameWorld.h"

class dskGameInterface;
//class GameWorldViewer;

/// Fenster, welches eine Sicherheitsabfrage vor dem Abreißen eines Gebäudes durchführt
class iwObservate : public IngameWindow
{
        GameWorldView* view;

        const unsigned short selected_x, selected_y;
        short last_x, last_y;

        // Scrolling
        bool scroll;
        int sx, sy;

    public:
        iwObservate(GameWorldViewer* const gwv, const unsigned short selected_x, const unsigned short selected_y);

    private:
        bool Draw_();
        void Msg_ButtonClick(const unsigned int ctrl_id);
        bool Msg_MouseMove(const MouseCoords& mc);
        bool Msg_RightDown(const MouseCoords& mc);
        bool Msg_RightUp(const MouseCoords& mc);
};

#endif // !iwOBSERVATE_H_INCLUDED
