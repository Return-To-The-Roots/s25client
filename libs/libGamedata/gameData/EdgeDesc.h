// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "DescIdx.h"
#include "Rect.h"
#include <string>

struct WorldDescription;
struct LandscapeDesc;
class CheckedLuaTable;

struct EdgeDesc
{
    std::string name;
    DescIdx<LandscapeDesc> landscape;
    std::string texturePath;
    Rect posInTexture;

    EdgeDesc(CheckedLuaTable luaData, const WorldDescription& worldDesc);
};
