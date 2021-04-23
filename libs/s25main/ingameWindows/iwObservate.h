// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

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
