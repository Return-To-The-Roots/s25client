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

#ifndef FoWNode_h__
#define FoWNode_h__

#include "gameTypes/MapTypes.h"
#include <array>
#include <cstdint>

class FOWObject;
class SerializedGameData;

/// Border stones on 1 node: Directly on Point and halfway to E, SE and SW
using BoundaryStones = std::array<uint8_t, 4>;

/// How a player sees the point in FoW
struct FoWNode
{
    /// Zeit (GF-Zeitpunkt), zu der, der Punkt zuletzt aktualisiert wurde
    unsigned last_update_time;
    /// Sichtbarkeit des Punktes
    Visibility visibility;
    /// FOW-Objekt
    FOWObject* object;
    std::array<uint8_t, 3> roads;
    unsigned char owner;
    BoundaryStones boundary_stones;

    FoWNode();
    void Serialize(SerializedGameData& sgd) const;
    void Deserialize(SerializedGameData& sgd);
};

#endif // FoWNode_h__
