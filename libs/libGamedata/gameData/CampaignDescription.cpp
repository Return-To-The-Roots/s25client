// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "CampaignDescription.h"
#include "lua/CheckedLuaTable.h"
#include "lua/LuaHelpers.h"

CampaignDescription::CampaignDescription() = default;

CampaignDescription::CampaignDescription(CheckedLuaTable luaData, CampaignDescription& campaignDesc)
{
    luaData.getOrThrow(campaignDesc.version, "version");
    luaData.getOrThrow(campaignDesc.author, "author");
    luaData.getOrThrow(campaignDesc.name, "name");
    luaData.getOrThrow(campaignDesc.shortDescription, "shortDescription");
    luaData.getOrThrow(campaignDesc.longDescription, "longDescription");
    luaData.getOrThrow(campaignDesc.maxHumanPlayers, "maxHumanPlayers");
    luaData.getOrThrow(campaignDesc.mapFolder, "mapFolder");
    luaData.getOrThrow(campaignDesc.luaFolder, "luaFolder");
    lua::validatePath(campaignDesc.mapFolder);
    lua::validatePath(campaignDesc.luaFolder);
    campaignDesc.mapNames = luaData.getOrDefault("maps", std::vector<std::string>());
    luaData.checkUnused();
}
