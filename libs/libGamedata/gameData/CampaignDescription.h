// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once
#include <string>
#include <vector>

class CheckedLuaTable;

struct CampaignDescription
{
    std::string version;
    std::string author;
    std::string name;
    std::string shortDescription;
    std::string longDescription;
    uint8_t maxHumanPlayers;
    std::string mapFolder;
    std::string luaFolder;
    std::vector<std::string> mapNames;

    CampaignDescription();
    CampaignDescription(CheckedLuaTable luaData, CampaignDescription& campaignDesc);
};
