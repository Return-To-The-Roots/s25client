// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>
#include <string>
#include <vector>

class CampaignSettings
{
public:
    CampaignSettings(std::string filePath);
    bool Load();
    void Save();

public:
    struct
    {
        std::string author;
        std::string name;
        std::string shortDescription;
        std::string longDescription;
        uint8_t maxPlayers;
    } campaignDescription;

    struct
    {
        std::vector<std::string> mapNames;
    } missions;

private:
    std::string filePath_;
    static const int VERSION;
    static const std::array<std::string, 2> SECTION_NAMES;
};
