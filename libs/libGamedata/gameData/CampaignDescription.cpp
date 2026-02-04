// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "CampaignDescription.h"
#include "RttrConfig.h"
#include "helpers/containerUtils.h"
#include "helpers/format.hpp"
#include "lua/CheckedLuaTable.h"
#include "lua/LuaHelpers.h"
#include "mygettext/mygettext.h"

CampaignDescription::CampaignDescription(const boost::filesystem::path& campaignPath, const kaguya::LuaRef& table)
{
    const auto resolveCampaignPath = [campaignPath](const std::string& path, bool isFolder) {
        const boost::filesystem::path tmpPath = path;
        if(tmpPath.is_relative())
        {
            // If it is only a file/folder name or empty use path relative to campaign folder
            if(!tmpPath.has_parent_path())
                return (campaignPath / tmpPath).string();
            if(!isFolder)
            {
                // For files only allow a single sub folder
                const auto parentPath = tmpPath.parent_path();
                if(!parentPath.parent_path().has_parent_path())
                {
                    // Only alpha-numeric folder names are allowed
                    const auto isAlNum = [](const char c) {
                        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9');
                    };
                    const auto isNonAlNum = [isAlNum](const char c) { return !isAlNum(c); };
                    lua::assertTrue(!helpers::contains_if(parentPath.string(), isNonAlNum),
                                    helpers::format(_("Invalid path '%1%': Must be alpha-numeric"), path));
                    return (campaignPath / tmpPath).string();
                }
            }
        }
        // Otherwise it must be a valid path inside the game files
        lua::validatePath(path);
        return path;
    };

    CheckedLuaTable luaData(table);
    luaData.getOrThrow(version, "version");
    luaData.getOrThrow(author, "author");
    luaData.getOrThrow(name, "name");
    luaData.getOrThrow(shortDescription, "shortDescription");
    luaData.getOrThrow(longDescription, "longDescription");
    const auto imageValue = luaData.getOptional<std::string>("image");
    if(imageValue && !imageValue->empty())
        image = resolveCampaignPath(*imageValue, false);

    luaData.getOrThrow(maxHumanPlayers, "maxHumanPlayers");

    if(maxHumanPlayers != 1)
        throw std::invalid_argument(helpers::format(_("Invalid maximum human player count: %1%"), maxHumanPlayers));

    luaData.getOrThrow(difficulty, "difficulty");

    if(difficulty != gettext_noop("easy") && difficulty != gettext_noop("medium") && difficulty != gettext_noop("hard"))
        throw std::invalid_argument(helpers::format(_("Invalid difficulty: %1%"), difficulty));

    const auto mapFolder = luaData.getOrDefault("mapFolder", std::string{});
    mapFolder_ = RTTRCONFIG.ExpandPath(resolveCampaignPath(mapFolder, true));
    // Default lua folder to map folder, i.e. LUA files are side by side with the maps
    luaFolder_ = RTTRCONFIG.ExpandPath(resolveCampaignPath(luaData.getOrDefault("luaFolder", mapFolder), true));
    mapNames_ = luaData.getOrDefault("maps", std::vector<std::string>());
    selectionMapData = luaData.getOptional<SelectionMapInputData>("selectionMap");
    if(selectionMapData)
    {
        const auto updatePath = [resolveCampaignPath](std::string& path) {
            if(!path.empty())
                path = resolveCampaignPath(path, false);
        };
        updatePath(selectionMapData->background.filePath);
        updatePath(selectionMapData->map.filePath);
        updatePath(selectionMapData->missionMapMask.filePath);
        updatePath(selectionMapData->marker.filePath);
        updatePath(selectionMapData->conquered.filePath);
    }
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
