// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "MapInfo.h"
#include "Savegame.h"

MapInfo::MapInfo()
{
    Clear();
}

MapInfo::~MapInfo() = default;

void MapInfo::Clear()
{
    type = MapType::OldMap;
    title.clear();
    filepath.clear();
    luaFilepath.clear();
    mapData.Clear();
    luaData.Clear();
    mapChecksum = 0;
    luaChecksum = 0;
    savegame.reset();
}
