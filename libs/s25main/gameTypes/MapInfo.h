// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "gameTypes/CompressedData.h"
#include "gameTypes/MapType.h"
#include <boost/filesystem/path.hpp>
#include <memory>
#include <string>

class Savegame;

class MapInfo
{
public:
    MapInfo();
    ~MapInfo();
    void Clear();

    /// Type of the map (savegame, old map)
    MapType type;
    /// Name of the map
    std::string title;
    /// Path where map is/will be stored
    boost::filesystem::path filepath;
    /// Path to lua file (if any)
    boost::filesystem::path luaFilepath;
    /// map data as received from server
    CompressedData mapData;
    /// lua script as received from server
    CompressedData luaData;
    /// Checksum of map data
    unsigned mapChecksum, luaChecksum;
    /// Savegame (set if type == MAP_SAVEGAME)
    std::unique_ptr<Savegame> savegame;
};
