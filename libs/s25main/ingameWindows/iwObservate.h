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
#include "gameTypes/MapCoordinates.h"

class GameWorldView;
class MouseCoords;

/// Observing window (shows a part of the world in an extra window)
class iwObservate : public IngameWindow
{
    /// View of parent GUI element
    GameWorldView& parentView;
    /// View shown in this window
    GameWorldView* view;

    const MapPoint selectedPt;
    DrawPoint lastWindowPos;

    // Scrolling
    bool isScrolling;
    Position scrollOrigin;

    unsigned zoomLvl;

    /// id of object currently followed or INVALID_ID
    unsigned followMovableId;

public:
    iwObservate(GameWorldView& gwv, MapPoint selectedPt);

private:
    void Draw_() override;
    void Msg_ButtonClick(unsigned ctrl_id) override;
    bool Msg_MouseMove(const MouseCoords& mc) override;
    bool Msg_RightDown(const MouseCoords& mc) override;
    bool Msg_RightUp(const MouseCoords& mc) override;
    /// Move view to the object we currently follow, return true if it can still be found
    bool MoveToFollowedObj();
    inline bool MoveToFollowedObj(MapPoint ptToCheck);
};
