// Copyright (c) 2017 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "Rect.h"
#include <helpers/EnumArray.h>
#include <string>

struct WorldDescription;
class CheckedLuaTable;

struct RoadTextureDesc
{
    std::string texturePath;
    Rect posInTexture;
};

enum class LandRoadType
{
    Normal,
    Upgraded,
    Boat,
    Mountain
};
constexpr auto maxEnumValue(LandRoadType)
{
    return LandRoadType::Mountain;
}

struct LandscapeDesc
{
    std::string name;
    std::string mapGfxPath;
    uint8_t s2Id;
    bool isWinter;
    helpers::EnumArray<RoadTextureDesc, LandRoadType> roadTexDesc;

    LandscapeDesc(CheckedLuaTable luaData, const WorldDescription& worldDesc);
};
