// Copyright (C) 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "controls/ctrlMapSelection.h"
#include "gameData/CampaignTypes.h"

#include <map>

struct CampaignDescription;

class CampaignSettings
{
public:
    void readSaveData(const CampaignID& campaignId, const std::string& saveString);
    std::map<CampaignID, std::string> createSaveData() const;

    bool isChapterPlayable(const CampaignDescription& campaignDesc, ChapterID chapter);
    std::vector<MissionStatus> getMissionsStatus(const CampaignDescription& campaignDesc);

    bool shouldShowVictoryScreen() const;
    auto getCompletedCampaign() const { return campaignCompleted_; }
    auto getCompletedChapter() const { return chapterCompleted_; }

    void enableChapter(CampaignID campaignUid, ChapterID chapter);
    void setChapterCompleted(CampaignID campaignUid, ChapterID chapter);
    void setCampaignCompleted(CampaignID campaignUid);
    void resetCompletionStatus();

private:
    enum class ChapterStatus
    {
        Disabled,
        Enabled,
        Completed
    };
    using CampaignState = std::map<ChapterID, ChapterStatus>;

    CampaignState& getCampaignState(const CampaignDescription& campaignDesc);
    std::string toSaveString(const CampaignState& state) const;
    void setChapterStatus(CampaignID campaignUid, ChapterID chapter, ChapterStatus status);

    std::map<CampaignID, CampaignState> states_;
    std::optional<ChapterID> chapterCompleted_;
    std::optional<CampaignID> campaignCompleted_;
};
