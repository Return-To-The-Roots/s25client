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
#ifndef iwOBSERVATE_H_INCLUDED
#define iwOBSERVATE_H_INCLUDED

#pragma once

#include "IngameWindow.h"
#include "gameTypes/MapTypes.h"

class GameWorldView;
class GameWorldViewer;
class MouseCoords;

/// Fenster, welches eine Sicherheitsabfrage vor dem Abreißen eines Gebäudes durchführt
class iwObservate : public IngameWindow
{
        GameWorldView& parentView;
        GameWorldView* view;

        const MapPoint selectedPt;
        short last_x, last_y;

        // Scrolling
        bool scroll;
        int sx, sy;

        unsigned zoomLvl;

    public:
        iwObservate(GameWorldView& gwv, const MapPoint selectedPt);

    private:
        bool Draw_() override;
        void Msg_ButtonClick(const unsigned int ctrl_id) override;
        bool Msg_MouseMove(const MouseCoords& mc) override;
        bool Msg_RightDown(const MouseCoords& mc) override;
        bool Msg_RightUp(const MouseCoords& mc) override;
};

#endif // !iwOBSERVATE_H_INCLUDED
