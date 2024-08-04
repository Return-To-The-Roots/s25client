// Copyright (C) 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "CampaignSaveData.h"
#include "Settings.h"
#include "gameData/CampaignDescription.h"
#include "gameData/CampaignSaveCodes.h"

namespace {
auto getCampaignSaveData(const CampaignDescription& campaignDesc)
{
    auto& saveData = SETTINGS.campaigns.saveData;

    if(!saveData.count(campaignDesc.uid))
        saveData[campaignDesc.uid] = campaignDesc.defaultChaptersEnabled;

    return saveData[campaignDesc.uid];
}
} // namespace

bool isChapterEnabled(const CampaignDescription& campaignDesc, unsigned char chapter)
{
    const auto& saveData = getCampaignSaveData(campaignDesc);

    if(chapter >= saveData.length())
        return false;

    return saveData[chapter] != CampaignSaveCodes::chapterDisabled;
}

std::vector<MissionStatus> getMissionsStatus(const CampaignDescription& campaignDesc)
{
    const auto& saveData = getCampaignSaveData(campaignDesc);

    std::vector<MissionStatus> ret(saveData.length());
    for(auto i = 0u; i < saveData.length(); ++i)
    {
        ret[i].playable = saveData[i] != CampaignSaveCodes::chapterDisabled;
        ret[i].conquered = saveData[i] == CampaignSaveCodes::chapterCompleted;
    }

    ret.resize(campaignDesc.getNumMaps());
    return ret;
}
