// Copyright (c) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
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

#include "DrawPoint.h"
#include <cstdint>

class SerializedGameData;

/// Type of a FoW object
enum class FoW_Type : uint8_t
{
    Nothing,
    Building,
    Buildingsite,
    Flag,
    Tree,
    Granite
};
constexpr auto maxEnumValue(FoW_Type)
{
    return FoW_Type::Granite;
}

/// Visual object in the Fog of War which shows what a player has seen there
class FOWObject
{
public:
    virtual ~FOWObject() = default;
    virtual void Draw(DrawPoint drawPt) const = 0;
    virtual void Serialize(SerializedGameData& sgd) const = 0;
    virtual FoW_Type GetType() const = 0;
};
