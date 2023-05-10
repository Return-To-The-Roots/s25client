// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "CampaingSettings.h"
#include "RTTR_assert.h"
#include "RttrConfig.h"
#include "libsiedler2/ArchivItem_Ini.h"
#include "libsiedler2/ErrorCodes.h"
#include "libsiedler2/libsiedler2.h"
#include "s25util/error.h"
#include <boost/filesystem/operations.hpp>
#include <regex>

const int CampaignSettings::VERSION = 1;
const std::array<std::string, 2> CampaignSettings::SECTION_NAMES = {{"description", "maps"}};

namespace bfs = boost::filesystem;

CampaignSettings::CampaignSettings(std::string filePath) : filePath_(filePath){};

bool CampaignSettings::Load()
{
    libsiedler2::Archiv settings;
    const auto settingsPath = RTTRCONFIG.ExpandPath(filePath_);
    try
    {
        if(int ec = libsiedler2::Load(settingsPath, settings) != 0)
            throw std::runtime_error(libsiedler2::getErrorString(ec));

        const libsiedler2::ArchivItem_Ini* iniDescription =
          static_cast<libsiedler2::ArchivItem_Ini*>(settings.find("description"));
        const libsiedler2::ArchivItem_Ini* iniMaps = static_cast<libsiedler2::ArchivItem_Ini*>(settings.find("maps"));

        if(!iniDescription || !iniMaps)
            throw std::runtime_error("Missing section");

        // stimmt die Settingsversion?
        if(iniDescription->getValue("version", 0) != VERSION)
            throw std::runtime_error("Wrong version");

        campaignDescription.author = iniDescription->getValue("author");
        campaignDescription.name = iniDescription->getValue("name");
        campaignDescription.shortDescription = iniDescription->getValue("short_description");
        campaignDescription.longDescription = iniDescription->getValue("long_description");
        campaignDescription.maxPlayers = iniDescription->getIntValue("max_human_players");

        auto mapList = iniMaps->getValue("maps");
        std::regex reg("\\,");

        missions.mapNames = std::vector<std::string>(
          std::sregex_token_iterator(mapList.begin(), mapList.end(), reg, -1), std::sregex_token_iterator());
    } catch(std::runtime_error& e)
    {
        s25util::warning(std::string("Could not use settings from \"") + settingsPath.string()
                         + "\". Reason: " + e.what());
        return false;
    }
    return true;
};

void CampaignSettings::Save()
{
    libsiedler2::Archiv settings;
    settings.alloc(SECTION_NAMES.size());
    for(unsigned i = 0; i < SECTION_NAMES.size(); ++i)
        settings.set(i, std::make_unique<libsiedler2::ArchivItem_Ini>(SECTION_NAMES[i]));

    libsiedler2::ArchivItem_Ini* iniDescription =
      static_cast<libsiedler2::ArchivItem_Ini*>(settings.find("description"));
    libsiedler2::ArchivItem_Ini* iniMaps = static_cast<libsiedler2::ArchivItem_Ini*>(settings.find("maps"));

    // ist eine der Kategorien nicht vorhanden?
    RTTR_Assert(iniDescription && iniMaps);

    // campaignDescription
    // {
    iniDescription->setValue("version", VERSION);
    iniDescription->setValue("author", campaignDescription.author);
    iniDescription->setValue("name", campaignDescription.name);
    iniDescription->setValue("short_description", campaignDescription.shortDescription);
    iniDescription->setValue("long_description", campaignDescription.longDescription);
    iniDescription->setValue("max_human_players", campaignDescription.maxPlayers);
    // };

    std::string mapList;
    for(auto const& element : missions.mapNames)
        mapList.append(element + ",");

    // missions
    // {
    iniMaps->setValue("maps", mapList);
    // };

    bfs::path settingsPath = RTTRCONFIG.ExpandPath(filePath_);
    if(libsiedler2::Write(settingsPath, settings) == 0)
        bfs::permissions(settingsPath, bfs::owner_read | bfs::owner_write);
};
