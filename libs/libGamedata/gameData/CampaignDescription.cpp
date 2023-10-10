// Copyright (C) 2005 - 2023 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "CampaignDescription.h"
#include "RttrConfig.h"
#include "helpers/format.hpp"
#include "lua/CheckedLuaTable.h"
#include "lua/LuaHelpers.h"
#include "mygettext/mygettext.h"

CampaignDescription::CampaignDescription(const kaguya::LuaRef& table)
{
    CheckedLuaTable luaData(table);
    luaData.getOrThrow(version, "version");
    luaData.getOrThrow(author, "author");
    luaData.getOrThrow(name, "name");
    luaData.getOrThrow(shortDescription, "shortDescription");
    luaData.getOrThrow(longDescription, "longDescription");
    luaData.getOrThrow(image, "image");
    luaData.getOrThrow(maxHumanPlayers, "maxHumanPlayers");

    if(maxHumanPlayers != 1)
        throw std::invalid_argument(helpers::format(_("Invalid maximum human player count: %1%"), maxHumanPlayers));

    luaData.getOrThrow(difficulty, "difficulty");

    if(difficulty != gettext_noop("easy") && difficulty != gettext_noop("medium") && difficulty != gettext_noop("hard"))
        throw std::invalid_argument(helpers::format(_("Invalid difficulty: %1%"), difficulty));

    luaData.getOrThrow(mapFolder, "mapFolder");
    luaData.getOrThrow(luaFolder, "luaFolder");
    lua::validatePath(mapFolder);
    lua::validatePath(luaFolder);
    mapNames = luaData.getOrDefault("maps", std::vector<std::string>());

    luaData.checkUnused();
}

size_t CampaignDescription::getNumMaps()
{
    return mapNames.size();
}

std::string CampaignDescription::getMapName(const size_t idx) const
{
    return mapNames.at(idx);
}

boost::filesystem::path CampaignDescription::getLuaFilePath(const size_t idx) const
{
    return (RTTRCONFIG.ExpandPath(luaFolder) / mapNames.at(idx)).replace_extension("lua");
}

boost::filesystem::path CampaignDescription::getMapFilePath(const size_t idx) const
{
    return RTTRCONFIG.ExpandPath(mapFolder) / mapNames.at(idx);
}
