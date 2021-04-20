// Copyright (c) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

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
