// Copyright (c) 2005 - 2015 Settlers Freaks (sf-team at siedler25.org)
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

#ifndef MapInfo_h__
#define MapInfo_h__

#include "GameProtocol.h"
#include "gameTypes/CompressedData.h"
#include "helpers/Deleter.h"
#include <boost/interprocess/smart_ptr/unique_ptr.hpp>
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
    std::string filepath;
    /// Path to lua file (if any)
    std::string luaFilepath;
    /// map data as received from server
    CompressedData mapData;
    /// lua script as received from server
    CompressedData luaData;
    /// Checksum of map data
    unsigned mapChecksum, luaChecksum;
    /// Savegame (set if type == MAP_SAVEGAME)
    boost::interprocess::unique_ptr<Savegame, Deleter<Savegame> > savegame;
};

#endif // MapInfo_h__
