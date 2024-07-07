// Copyright (C) 2005 - 2023 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "DrawPoint.h"
#include "Timer.h"
#include "gameTypes/MapCoordinates.h"

class GameWorldView;

class PickedMovableObject
{
public:
    static PickedMovableObject pick(const GameWorldView& gwv, MapPoint mapPt, DrawPoint drawPt, bool expire);
    static PickedMovableObject pickAtCursor(const GameWorldView& gwv, bool expire);
    static PickedMovableObject pickAtViewCenter(const GameWorldView& gwv, bool expire);

    PickedMovableObject() = default;

    unsigned id() const { return id_; }
    bool isValid() const;

    void cancelExpiration();
    void invalidate();

    bool track(const GameWorldView& gwv);
    bool track(GameWorldView& gwv, bool moveTo);

private:
    unsigned id_ = 0;
    MapPoint mapPt_{};
    DrawPoint drawPt_{};
    Timer expiration_{};
};
