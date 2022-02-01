// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "MapInfo.h"
#include "Savegame.h"
#include <limits>

/// Hard upper limit on the file size of maps/savegames, currently: 500MB
constexpr size_t MAX_FILE_SIZE = 100 * 1024 * 1024;
// The summed value of map + lua must at least fit into an uint32, also we have it twice: compressed and uncompressed
static_assert(MAX_FILE_SIZE <= std::numeric_limits<uint32_t>::max() / 4u, "Max size to large");

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

bool MapInfo::verifySize() const
{
    return verifySize(mapData.data.size(), luaData.data.size(), mapData.uncompressedLength, luaData.uncompressedLength);
}

bool MapInfo::verifySize(size_t mapLen, size_t mapLenCompressed, size_t luaLen, size_t luaLenCompressed)
{
    for(const auto size : {mapLen, mapLenCompressed, luaLen, luaLenCompressed})
    {
        if(size > MAX_FILE_SIZE)
            return false;
    }
    return true;
}
