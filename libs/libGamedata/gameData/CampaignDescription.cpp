// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "CampaignDescription.h"
#include "RttrConfig.h"
#include "helpers/format.hpp"
#include "lua/CheckedLuaTable.h"
#include "lua/LuaHelpers.h"
#include "mygettext/mygettext.h"

CampaignDescription::CampaignDescription(const boost::filesystem::path& campaignPath, const kaguya::LuaRef& table)
{
    CheckedLuaTable luaData(table);
    luaData.getOrThrow(version, "version");
    luaData.getOrThrow(author, "author");
    luaData.getOrThrow(name, "name");
    luaData.getOrThrow(shortDescription, "shortDescription");
    luaData.getOrThrow(longDescription, "longDescription");
    image = luaData.getOptional<std::string>("image");
    if(image && image->empty())
        image = std::nullopt;
    luaData.getOrThrow(maxHumanPlayers, "maxHumanPlayers");

    if(maxHumanPlayers != 1)
        throw std::invalid_argument(helpers::format(_("Invalid maximum human player count: %1%"), maxHumanPlayers));

    luaData.getOrThrow(difficulty, "difficulty");

    if(difficulty != gettext_noop("easy") && difficulty != gettext_noop("medium") && difficulty != gettext_noop("hard"))
        throw std::invalid_argument(helpers::format(_("Invalid difficulty: %1%"), difficulty));

    auto resolveFolder = [campaignPath](const std::string& folder) {
        const boost::filesystem::path tmpPath = folder;
        // If it is only a filename or empty use path relative to campaign folder
        if(!tmpPath.has_parent_path())
            return campaignPath / tmpPath;
        // Otherwise it must be a valid path inside the game files
        lua::validatePath(folder);
        return RTTRCONFIG.ExpandPath(folder);
    };

    const auto mapFolder = luaData.getOrDefault("mapFolder", std::string{});
    mapFolder_ = resolveFolder(mapFolder);
    // Default lua folder to map folder, i.e. LUA files are side by side with the maps
    luaFolder_ = resolveFolder(luaData.getOrDefault("luaFolder", mapFolder));
    mapNames_ = luaData.getOrDefault("maps", std::vector<std::string>());
    selectionMapData = luaData.getOptional<SelectionMapInputData>("selectionMap");
    luaData.checkUnused();
}

boost::filesystem::path CampaignDescription::getLuaFilePath(const size_t idx) const
{
    return (luaFolder_ / getMapName(idx)).replace_extension("lua");
}

boost::filesystem::path CampaignDescription::getMapFilePath(const size_t idx) const
{
    return mapFolder_ / getMapName(idx);
}
