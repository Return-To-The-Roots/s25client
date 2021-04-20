// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "noBase.h"
#include "gameTypes/MapCoordinates.h"
class SerializedGameData;

class noCoordBase : public noBase
{
public:
    noCoordBase(const NodalObjectType nop, const MapPoint pt) : noBase(nop), pos(pt) {}
    noCoordBase(SerializedGameData& sgd, unsigned obj_id);

    void Serialize(SerializedGameData& sgd) const override;

    /// liefert die X-Koordinate.
    MapCoord GetX() const { return pos.x; }
    /// liefert die Y-Koordinate.
    MapCoord GetY() const { return pos.y; }

    /// Returns position
    MapPoint GetPos() const { return pos; }

protected:
    MapPoint pos;
};
